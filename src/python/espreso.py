
import numpy as np 
from scipy import sparse 
import myModul as mM
import config_espreso_python  
import pylab as plt
import scipy.sparse.linalg as spla
import multiprocessing






n_clus          = 2
n_subPerClust   = 8


CONSTANT_89 = 24 


problem_info = {'n_clus': n_clus,'n_subPerClust':n_subPerClust}

path = '../log/'





def readClusterData(x):    
    i = x[0];    j = x[1]    
    y=[]    
    #0 mat_K
    y.append(mM.load_matrix(path,'K',i,j,makeSparse=True,makeSymmetric=False))
    #1 mat_Kreg    
    y.append(mM.load_matrix(path,'Kreg',i,j,makeSparse=True,makeSymmetric=True))      
    #2 mat_B0    
    y.append(mM.load_matrix(path,'B0',i,j,makeSparse=True,makeSymmetric=False))
    #3 mat_B1    
    y.append(mM.load_matrix(path,'B1',i,j,makeSparse=True,makeSymmetric=False))
    #4 mat_R
    y.append(mM.load_matrix(path,'R',i,j,makeSparse=True,makeSymmetric=False))
    #5 vec_f    
    y.append(mM.load_vector(path,'f',i,j))
    #6 vec_c    
    y.append(mM.load_vector(path,'c',i,j))
    #7vec_weight    
    y.append(mM.load_vector(path,'weight',i,j))
    tmp = mM.load_vector(path,'loc_ind_weight',i,j)
    tmp = tmp.astype(np.int32)
    #8 vec_index_weight
    y.append(tmp)
    return y
    
    
def getDirichletPrecond(x): 
    i = x[0];    j = x[1]    
    #_B1 = x[2].copy()
    mat_K_ij = x[3] 
    J = np.unique(x[2].tocoo().col)            
    if config_espreso_python.Dirichlet_from_espreso:
        #mat_Schur_Dirichlet[i].append([J,mM.load_matrix(path,'S',i,j,makeSparse=False,makeSymmetric=True)])
        S = mM.load_matrix(path,'S',i,j,makeSparse=False,makeSymmetric=True)
    else:
        I = np.arange(0,mat_K_ij.shape[0])
        I = np.delete(I,J)
        K_II = mat_K_ij[np.array(I)[:,np.newaxis],np.array(I)].tocsc()           
        K_IJ = mat_K_ij[np.array(I)[:,np.newaxis],np.array(J)].toarray()
        K_JJ = mat_K_ij[np.array(J)[:,np.newaxis],np.array(J)].toarray()
        iK_II = spla.splu(K_II)
        iK_II_K_IJ = np.zeros(K_IJ.shape)
        for k in range(K_IJ.shape[1]):
            iK_II_K_IJ[:,k] = iK_II.solve(K_IJ[:,k])
        S = K_JJ-np.dot(K_IJ.transpose(),iK_II_K_IJ)
    print('Dir. precond: ',i,j)
    return J, S


def creat_matB0_ker_from_IJV(ijv_B0ker):

    #mat_B0ker.append([])
    #n = np.max(ijv_B0ker[-1][0])+1 
    I = ijv_B0ker[0][1::]
    J = ijv_B0ker[1][1::] 
    V = ijv_B0ker[2][1::]
    n = ijv_B0ker[3][0]
    m = ijv_B0ker[3][1]
    return sparse.csc_matrix((V,(I,J)),shape=(m,n)).tocsc()



mat_K       = []
mat_Salfa   = []
mat_Kreg    = []
mat_B0      = []
mat_B1      = []
mat_R       = []
vec_f       = []
vec_c       = []
vec_weight  = []
vec_index_weight = []
mat_Schur_Dirichlet   = []


if config_espreso_python.flag_multiprocessing:
    pool = multiprocessing.Pool()
    
for i in range(n_clus): 
    mat_K.append([])
    mat_Kreg.append([])
    mat_B0.append([])
    mat_B1.append([])
    mat_R.append([])
    vec_f.append([])
    vec_c.append([])
    vec_weight.append([])
    vec_index_weight.append([])

#    mat_Salfa.append(mM.load_matrix(path,'Salfa',0,'',makeSparse=False,makeSymmetric=True))
    mat_Salfa.append([])
#    for j in range(n_subPerClust):  
    ij = []
    for j in range(n_subPerClust):
        ij.append([i,j])
    if config_espreso_python.flag_multiprocessing:
        k = pool.map(readClusterData,ij)
    else:
        k = list(map(readClusterData,ij))
        
    for j in range(n_subPerClust):
        mat_K[i].append(k[j][0])
        mat_Kreg[i].append(k[j][1])
        mat_B0[i].append(k[j][2])
        mat_B1[i].append(k[j][3])
        mat_R[i].append(k[j][4])
        vec_f[i].append(k[j][5])
        vec_c[i].append(k[j][6])
        vec_weight[i].append(k[j][7])
        vec_index_weight[i].append(k[j][8])
        print('.',end='')
    
    #print(k[0][0])
print('done')
    
        
###############################################################################
####################### FETI PREPROCESSING ####################################
###############################################################################   
conf = config_espreso_python
if conf.precondDualSystem=='dirichlet':
    for i in range(len(mat_K)): 
        ij = []
        for j in range(len(mat_K[i])):
            ij.append([i,j,mat_B1[i][j],mat_K[i][j]]) 
        if config_espreso_python.flag_multiprocessing:
            mat_Schur_Dirichlet.append(pool.map(getDirichletPrecond,ij)) 
        else:
            mat_Schur_Dirichlet.append(list(map(getDirichletPrecond,ij)))


ijv_B0ker     = []          
for i in range(len(mat_K)):
    ijv_B0ker.append([]) 
    for j in range(len(mat_K[i])):
        ijv_B0ker[i].append([np.array([0]),np.array([0]),np.array([0]),\
            np.array([0,0])]) 
if True:
    for i in range(len(mat_B0)):
        cnt_ijv = 0
        for j in range(len(mat_B0[i])-1):
            for k in range(j+1,len(mat_B0[i])):
                tmpB_jk = sparse.hstack([np.abs(mat_B1[i][j]),np.abs(mat_B1[i][k])])
                tmpB_jk = tmpB_jk.astype(bool)
                tmpB_jk = tmpB_jk.astype(int)
                indx = np.ravel(tmpB_jk.sum(1)==2) 

                if (np.sum(indx)>CONSTANT_89):
#                   print('YES ............... clust: ',i,'--',j,k, np.sum(indx))
#                    
                    iB0_j = mat_B1[i][j].tocsr()[indx,:].indices
                    iB0_k = mat_B1[i][k].tocsr()[indx,:].indices

                    R_g_j = mat_R[i][j].toarray()[iB0_j,:]
                    R_g_k = mat_R[i][k].toarray()[iB0_k,:]
#
                    if True:
                        QQ,RR = np.linalg.qr(np.vstack((R_g_j,R_g_k)))
                        log_ind = np.abs(np.diag(RR))>1e-10
                        
                        
                        if np.sum(log_ind)<log_ind.shape[0]:
                            nR = R_g_j.shape[0]
                            print('B0 reduced -------------------------')
                            R_g_j = QQ[:nR,log_ind];  R_g_k = QQ[nR:,log_ind]
                    
                    _nj = iB0_j.shape[0];  _nk = iB0_k.shape[0]
                    
                    for l in range(R_g_j.shape[1]):

                        _i_j = cnt_ijv*np.ones(_nj,dtype=np.int32)
                        _j_j = iB0_j
                        _v_j = R_g_j[:,l]
                        ijv_B0ker[i][j][0] = np.concatenate((ijv_B0ker[i][j][0],_i_j))
                        ijv_B0ker[i][j][1] = np.concatenate((ijv_B0ker[i][j][1],_j_j))
                        ijv_B0ker[i][j][2] = np.concatenate((ijv_B0ker[i][j][2],_v_j))
                                                
                        _i_k = cnt_ijv*np.ones(_nk,dtype=np.int32)
                        _j_k = iB0_k
                        _v_k = -R_g_k[:,l]
                        ijv_B0ker[i][k][0] = np.concatenate((ijv_B0ker[i][k][0],_i_k))
                        ijv_B0ker[i][k][1] = np.concatenate((ijv_B0ker[i][k][1],_j_k))
                        ijv_B0ker[i][k][2] = np.concatenate((ijv_B0ker[i][k][2],_v_k))
                                                
                        cnt_ijv += 1
#                else:
#                   print('NO  ............... clust: ',i,'--',j,k,'\t\t',np.sum(indx))                                
    mat_B0ker      = []                    
    for i in range(len(mat_B0)):
        n_lam = np.max(ijv_B0ker[i][-1][0])+1 
        for j in range(len(mat_B0[i])):
            nDOFs = mat_K[i][j].shape[0] 
            ijv_B0ker[i][j][3] = np.array([nDOFs,n_lam])  
        if config_espreso_python.flag_multiprocessing:
            mat_B0ker.append(pool.map(creat_matB0_ker_from_IJV,ijv_B0ker[i]))
        else:
            mat_B0ker.append(list(map(creat_matB0_ker_from_IJV,ijv_B0ker[i])))
    #mat_B0 = mat_B0ker
            
if config_espreso_python.flag_multiprocessing:
    pool.close()

print('\nTFETI')
u,lam = mM.feti(mat_K,mat_Kreg,vec_f,mat_Schur_Dirichlet,mat_B1,vec_c,vec_weight,\
                            vec_index_weight,mat_R)
                        

print('\nHFETI - corners')
uHDPc,lamHc = mM.hfeti(mat_K,mat_Kreg,vec_f,mat_Schur_Dirichlet,mat_B0,mat_B1,vec_c,\
                        vec_weight,vec_index_weight,mat_R,mat_Salfa)
print('size(F0) =',mat_B0[0][0].shape[0],' (corners)')
print('\nHFETI - kernels')    
uHDP,lamH= mM.hfeti(mat_K,mat_Kreg,vec_f,mat_Schur_Dirichlet,mat_B0ker,mat_B1,vec_c,\
                        vec_weight,vec_index_weight,mat_R,mat_Salfa)   
                        
print('size(F0) =',mat_B0ker[0][0].shape[0],' (kernels)')
                        
                        
#
norm_del_u = 0
norm_u = 0
for i in range(len(u)):
    for j in range(len(u[i])):
        norm_del_u += np.linalg.norm(uHDPc[i][j]-uHDP[i][j])
        norm_u += np.linalg.norm(u[i][j])

print('|u_TFETI-u_HTFETI|/|u_TFETI| = ',norm_del_u/norm_u)


#conf.iterative_Kplus=False
#uHDP,lamH = mM.hfeti(K,Kreg,f,B0,B1,R,weight)
#
#methodToImproveSolByIterMethod      = 'cg_dx'
#conf.precondPrimalSystem            = 'diag' 
#conf.iterative_Kplus=True
#uHSP,lamH = mM.hfeti(K,Kreg,f,B0,B1,R,weight)



#ndu = np.linalg.norm(uHSP-uHDP)
#nu = np.linalg.norm(uHDP)
#print('||uHDP-uHSP||/||uHDP||       = ',ndu/nu)

#ndu = np.linalg.norm(u-uH)
#nu = np.linalg.norm(u)
#ndlam = np.linalg.norm(lam-lamH)
#nlam = np.linalg.norm(lam)
#print('||u-uH||/||u||       = ',ndu/nu)
#print('||lam-lamH||/||lam|| = ',ndlam/nlam)