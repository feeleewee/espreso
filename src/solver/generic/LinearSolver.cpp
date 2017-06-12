/*
 * LinearSolver.cpp
 *
 *  Created on: Sep 24, 2015
 *      Author: lriha
 */

#include "LinearSolver.h"

//#include <Eigen/Dense>
//using Eigen::MatrixXd;

using namespace espreso;

LinearSolver::LinearSolver(Instance *instance, const ESPRESOSolver &configuration)
: instance(instance),
  configuration(configuration),
  physics(NULL),
  constraints(NULL),
  timeEvalMain("ESPRESO Solver Overal Timing"),
  cluster(NULL),
  solver(NULL)
{
}


LinearSolver::LinearSolver(const ESPRESOSolver &configuration, OldPhysics &physics, Constraints &constraints)
: instance(NULL),
  configuration(configuration),
  physics(&physics),
  constraints(&constraints),
  timeEvalMain("ESPRESO Solver Overal Timing")
{
//	cluster  = new SuperCluster(configuration, instance);
//	solver = new IterSolver(configuration);
}

LinearSolver::~LinearSolver() {

	if (cluster != NULL) {
		;//delete cluster;
	}

	if (solver != NULL) {
		;//delete solver;
	}
}


// make full initialization of solver
void LinearSolver::init()
{
	if (cluster != NULL) {
		delete cluster;
	}

	if (solver != NULL) {
		delete solver;
	}

	solver  = new IterSolver	(configuration);
	cluster = new SuperCluster	(configuration, instance);

	init(instance->neighbours);

}

// make partial initialization according to updated matrices
void LinearSolver::update(Matrices matrices)
{
	// TODO update appropriate solver objects and stop steeling matrices! :)

	if (matrices & Matrices::K) {
		// factorization and preconditioners and HFETI preprocessing

		delete cluster;
		delete solver;

		cluster = new SuperCluster(configuration, instance);
		solver  = new IterSolver(configuration);

		init(instance->neighbours);

	}

	if (matrices & (Matrices::N | Matrices::B1)) { // N is kernel of matrix K
		// updateGGt();
		setup_CreateG_GGt_CompressG();
	}

	if (matrices & (Matrices::B1c | Matrices::f)) {
		// update dual RHS
		//for f:
		// - updated by solve

		//for B1c:
		setup_SetDirichletBoundaryConditions();
	}

	if (matrices & Matrices::B0) {
		// HFETI preprocessing
		setup_HTFETI();
	}

	if (matrices & Matrices::B1duplicity) {
		// update duplicity vector
		#pragma omp parallel for
		for (eslocal d = 0; d < cluster->domains.size(); d++) {
			cluster->domains[d]->B1_scale_vec = instance->B1duplicity[d];
		}
	}

	// TODO: remove full re-initialization
	//	delete cluster;
	//	delete solver;
	//	cluster = new Cluster(configuration);
	//	solver = new IterSolver(configuration);
	//	setup();
	//	init(instance->neighbours);
}

// run solver and store primal and dual solution
void LinearSolver::run()
{
	Solve(instance->f, instance->primalSolution, instance->dualSolution);
}


void LinearSolver::setup_HTFETI() {
// Setup Hybrid FETI part of the solver

	if (cluster->USE_HFETI == 1) {
		 TimeEvent timeHFETIprec(string("Solver - HFETI preprocessing"));
		 timeHFETIprec.start();

		cluster->SetClusterHFETI();

		 timeHFETIprec.endWithBarrier();
		 timeEvalMain.addEvent(timeHFETIprec);

		ESLOG(MEMORY) << "After HFETI preprocessing process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";
	}
}

void LinearSolver::setup_LocalSchurComplement() {
// Computation of the Local Schur Complements
//TODO: update for multiple clusters

	if (cluster->USE_KINV == 1) {
		 TimeEvent KSCMem(string("Solver - SC asm. w PARDISO-SC mem [MB]")); KSCMem.startWithoutBarrier(GetProcessMemory_u());
		 TimeEvent timeSolSC2(string("Solver - Schur Complement asm. - using PARDISO-SC"));timeSolSC2.start();
		bool USE_FLOAT = false;
		if (configuration.schur_precision == FLOAT_PRECISION::SINGLE) {
			USE_FLOAT = true;
		}
		cluster->Create_SC_perDomain(USE_FLOAT);
		 timeSolSC2.endWithBarrier(); timeEvalMain.addEvent(timeSolSC2);
		 KSCMem.endWithoutBarrier(GetProcessMemory_u()); //KSCMem.printLastStatMPIPerNode();
		 ESLOG(MEMORY) << "After K inv. process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";
	} else {
		for (size_t d = 0; d < cluster->domains.size(); d++) {
			cluster->domains[d]->isOnACC = 0;
		}
	}

}

void LinearSolver::setup_Preconditioner() {
// Load Matrix K, Regularization
		 TimeEvent timeRegKproc(string("Solver - Setup preconditioners")); timeRegKproc.start();
		 ESLOG(MEMORY) << "Before - Setup preconditioners - process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

		 TimeEvent KregMem(string("Solver - Setup preconditioners mem. [MB]")); KregMem.startWithoutBarrier(GetProcessMemory_u());
		 ESINFO(PROGRESS3) << "Setup preconditioners";

		cluster->SetupPreconditioner();

		 KregMem.endWithoutBarrier(GetProcessMemory_u()); //KregMem.printLastStatMPIPerNode();

		 ESLOG(MEMORY) << "After - Setup preconditioners " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";
		 timeRegKproc.endWithBarrier();
		 timeEvalMain.addEvent(timeRegKproc);
}

void LinearSolver::setup_FactorizationOfStiffnessMatrices() {
// K Factorization
		 TimeEvent timeSolKproc(string("Solver - K factorization")); timeSolKproc.start();
		 TimeEvent KFactMem(string("Solver - K factorization mem. [MB]")); KFactMem.startWithoutBarrier(GetProcessMemory_u());
		 ESINFO(PROGRESS3) << "Factorize K";

		cluster->SetupKsolvers();

		 KFactMem.endWithoutBarrier(GetProcessMemory_u()); //KFactMem.printLastStatMPIPerNode();
		 ESLOG(MEMORY) << "After K solver setup process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";
		 timeSolKproc.endWithBarrier();
		 timeEvalMain.addEvent(timeSolKproc);
}


void LinearSolver::setup_SetDirichletBoundaryConditions() {
// Set Dirichlet Boundary Condition

		 TimeEvent timeSetInitialCondition(string("Solver - Set Dirichlet Boundary Condition"));
		 timeSetInitialCondition.start();

		#pragma omp parallel for
		for (int d = 0; d < cluster->domains.size(); d++) {
			cluster->domains[d]->vec_c  = instance->B1c[d];
			cluster->domains[d]->vec_lb = instance->LB[d];
		}

		 timeSetInitialCondition.endWithBarrier();
		 timeEvalMain.addEvent(timeSetInitialCondition);
}

void LinearSolver::setup_CreateG_GGt_CompressG() {

		 TimeEvent timeSolPrec(string("Solver - FETI Preprocessing")); timeSolPrec.start();

		 ESLOG(MEMORY) << "Solver Preprocessing - HFETI with regularization from K matrix";
		 ESLOG(MEMORY) << "process " << environment->MPIrank << " uses "<< Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

		 TimeEvent G1_perCluster_time("Setup G1 per Cluster time   - preprocessing"); G1_perCluster_time.start();

		 TimeEvent G1_perCluster_mem("Setup G1 per Cluster memory - preprocessing"); G1_perCluster_mem.startWithoutBarrier(GetProcessMemory_u());
		cluster->Create_G_perCluster();

		 G1_perCluster_time.end(); G1_perCluster_time.printStatMPI();
		 G1_perCluster_mem.endWithoutBarrier(GetProcessMemory_u()); G1_perCluster_mem.printStatMPI();

		 ESLOG(MEMORY) << "Created G1 per cluster";
		 ESLOG(MEMORY) << "Before HFETI create GGt process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

		 TimeEvent solver_Preprocessing_time("Setup GGt time   - preprocessing"); solver_Preprocessing_time.start();
		 TimeEvent solver_Preprocessing_mem("Setup GGt memory - preprocessing"); solver_Preprocessing_mem.start();

		solver->Preprocessing(*cluster);

		 solver_Preprocessing_time.end(); solver_Preprocessing_time.printStatMPI();
		 solver_Preprocessing_mem.end();  solver_Preprocessing_mem.printStatMPI();

		 ESLOG(MEMORY) << "Create GGtInv";
		 ESLOG(MEMORY) << "process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

		 TimeEvent solver_G1comp_time("Setup G1 compression time   - preprocessing"); solver_G1comp_time.start();
		 TimeEvent solver_G1comp_mem("Setup G1 compression memory - preprocessing");  solver_G1comp_mem.start();

		cluster->Compress_G1(); // Compression of Matrix G1 to work with compressed lambda vectors

		 solver_G1comp_time.end(); solver_G1comp_time.printStatMPI();
		 solver_G1comp_mem.end();  solver_G1comp_mem.printStatMPI();
		 ESLOG(MEMORY) << "G1 compression";
		 ESLOG(MEMORY) << "process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
		 ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

		 timeSolPrec.endWithBarrier(); timeEvalMain.addEvent(timeSolPrec);


}



void LinearSolver::setup_InitClusterAndSolver( )
{

	cluster->SetupCommunicationLayer();


	// *** Iter Solver Set-up ***************************************************************************************************************************
	solver->CG_max_iter 		= configuration.iterations;
	solver->USE_GGtINV  		= 1;
	solver->epsilon 			= configuration.epsilon;
	solver->USE_PREC 			= configuration.preconditioner;

	solver->USE_KINV 			= configuration.use_schur_complement ? 1 : 0;
	solver->PAR_NUM_THREADS 	= environment->PAR_NUM_THREADS;
	solver->SOLVER_NUM_THREADS  = environment->SOLVER_NUM_THREADS;

	switch (configuration.method) {
	case ESPRESO_METHOD::TOTAL_FETI:
		solver->USE_HFETI = false;
		break;
	case ESPRESO_METHOD::HYBRID_FETI:
		solver->USE_HFETI = true;
		break;
	default:
		ESINFO(GLOBAL_ERROR) << "Unsupported FETI METHOD";
	}


}

// TODO: const parameters
void LinearSolver::init(const std::vector<int> &neighbours)
{
	if (physics != NULL) {
		instance = new Instance(physics->K.size(), neighbours);

		physics->K.swap(instance->K);
		for (size_t i = 0; i < physics->K.size(); i++) {
			instance->K[i].mtype = physics->mtype;
		}

		physics->R1.swap(instance->N1);
		physics->R2.swap(instance->N2);
		physics->RegMat.swap(instance->RegMat);

		constraints->B1.swap(instance->B1);
		constraints->B1c.swap(instance->B1c);
		constraints->B1subdomainsMap.swap(instance->B1subdomainsMap);
		constraints->B1clustersMap.swap(instance->B1clustersMap);
		constraints->B1duplicity.swap(instance->B1duplicity);

		constraints->B0.swap(instance->B0);
		constraints->B0subdomainsMap.swap(instance->B0subdomainsMap);

		constraints->LB.swap(instance->LB);
		constraints->inequality.swap(instance->inequality);
		constraints->inequalityC.swap(instance->inequalityC);

		cluster->instance  = instance;
		//SCluster->instance = instance;

	}

	//mkl_cbwr_set(MKL_CBWR_COMPATIBLE);

	// Overall Linear Solver Time measurement structure
	timeEvalMain.totalTime.startWithBarrier();



	// *** Initialize the Cluster and Solver structures
	setup_InitClusterAndSolver();

	// *** Set Dirichlet Boundary Condition
	// setup_SetDirichletBoundaryConditions();

	// *** if TFETI is used or if HTFETI and analytical kernel are used we can compute GGt here - between solution in terms of peak memory
	if (  !(cluster->USE_HFETI == 1 && configuration.regularization == REGULARIZATION::NULL_PIVOTS)  ) {
		setup_CreateG_GGt_CompressG();
	}

	// *** setup all preconditioners
	setup_Preconditioner();

	// *** Computation of the Schur Complement
	setup_LocalSchurComplement();

	// *** K Factorization
	setup_FactorizationOfStiffnessMatrices();

	// *** Setup Hybrid FETI part of the solver
	setup_HTFETI();

	// *** For algebraic kernels, GGt needs to be computed after HTFETI preprocessing
	if (cluster->USE_HFETI == 1 && configuration.regularization == REGULARIZATION::NULL_PIVOTS) {
		setup_CreateG_GGt_CompressG();
	}


	// Cleanup of unnecessary objects
	// TODO: This can be a problem in some cases - need to be verified
	cluster->my_lamdas_map_indices.clear();

	#pragma omp parallel for
	for (size_t d = 0; d < cluster->domains.size(); d++) {
		cluster->domains[d]->B1.Clear();
	}

	ESLOG(MEMORY) << "End of preprocessing - process " << environment->MPIrank << " uses " << Measure::processMemory() << " MB";
	ESLOG(MEMORY) << "Total used RAM " << Measure::usedRAM() << "/" << Measure::availableRAM() << " [MB]";

}

void LinearSolver::Solve( std::vector < std::vector < double > >  & f_vec,
		                  std::vector < std::vector < double > >  & prim_solution)
{

	std::vector < std::vector < double > > dual_solution;
	Solve(f_vec, prim_solution, dual_solution);
}

void LinearSolver::Solve( std::vector < std::vector < double > >  & f_vec,
		                  std::vector < std::vector < double > >  & prim_solution,
		                  std::vector < std::vector < double > > & dual_solution) {

	 	 TimeEvent timeSolCG(string("Solver - CG Solver runtime"));
	 	 timeSolCG.start();

	 solver->Solve    ( *cluster, f_vec, prim_solution, dual_solution );

	 	 timeSolCG.endWithBarrier();
	 	 timeEvalMain.addEvent(timeSolCG);

}

void LinearSolver::Postprocessing( ) {

}

void LinearSolver::finilize() {

	// Show Linear Solver Runtime Evaluation
	solver->preproc_timing.printStatsMPI();
	solver->timing.printStatsMPI();
	solver->postproc_timing.printStatsMPI();
	solver->timeEvalAppa.printStatsMPI();
	solver->timeEvalProj.printStatsMPI();

	if ( solver->USE_PREC != ESPRESO_PRECONDITIONER::NONE ) solver->timeEvalPrec.printStatsMPI();

	//TODO: Fix timing:  if ( cluster->USE_HFETI == 1 ) cluster->ShowTiming();

	 timeEvalMain.totalTime.endWithBarrier();
	 timeEvalMain.printStatsMPI();
}

void LinearSolver::CheckSolution( vector < vector < double > > & prim_solution ) {
    // *** Solutin correctnes test **********************************************************************************************

	//	double max_v = 0.0;
//		for (eslocal i = 0; i < number_of_subdomains_per_cluster; i++)
//			for (size_t j = 0; j < prim_solution[i].size(); j++)
//				if ( fabs ( prim_solution[i][j] ) > max_v) max_v = fabs( prim_solution[i][j] );
//
//	TimeEvent max_sol_ev ("Max solution value "); max_sol_ev.startWithoutBarrier(0.0); max_sol_ev.endWithoutBarrier(max_v);
//
//	double max_vg;
//	MPI_Reduce(&max_v, &max_vg, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
//	ESINFO(DETAILS) << "Maxvalue in solution = " << std::setprecision(12) << max_vg;

	//max_sol_ev.printLastStatMPIPerNode(max_vg);
	// *** END - Solutin correctnes test ******************************************************************************************

}

void LinearSolver::Preprocessing( std::vector < std::vector < eslocal > > & lambda_map_sub) {

}
