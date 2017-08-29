
#ifndef SRC_CONFIG_OBJECTHOLDER_H_
#define SRC_CONFIG_OBJECTHOLDER_H_

#include "configuration.h"
#include "valueholder.h"

namespace espreso {

template <typename TParameter, typename TValue>
struct ECFValueMap: public ECFObject {
	std::map<TParameter, TValue> &value;

	ECFValueMap(std::map<TParameter, TValue> &value): value(value) {}

	virtual ECFParameter* getParameter(const std::string &name)
	{
		if (ECFObject::getParameter(name) != NULL) {
			return ECFObject::getParameter(name);
		}
		TParameter key;
		if (ECFValueHolder<TParameter>(key).setValue(name)) {
			return registerParameter(name, new ECFValueHolder<TValue>(value[key]), metadata.suffix(1));
		} else {
			return NULL;
		}
	}

	virtual const ECFParameter* getPattern() const
	{
		ECFParameter *parameter = new ECFValueHolder<TValue>(_patternValue);
		parameter->setValue(metadata.pattern[1]);
		return registerPatternParameter(parameter);
	}

private:
	mutable TValue _patternValue;
};

template <typename TParameter, typename TValue>
struct ECFValueMultiMap: public ECFObject {
	std::multimap<TParameter, TValue> &value;

	ECFValueMultiMap(std::multimap<TParameter, TValue> &value): value(value) {}

	virtual ECFParameter* getParameter(const std::string &name)
	{
		// TODO: this assume that getParameter always set a correct value
		TParameter key;
		if (ECFValueHolder<TParameter>(key).setValue(name)) {
			auto it = value.insert(std::make_pair(key, TValue{}));
			return registerParameter(name, new ECFValueHolder<TValue>(it->second), metadata.suffix(1));
		} else {
			return NULL;
		}
	}

	virtual const ECFParameter* getPattern() const
	{
		ECFParameter *parameter = new ECFValueHolder<TValue>(_patternValue);
		parameter->setValue(metadata.pattern[1]);
		return registerPatternParameter(parameter);
	}

private:
	mutable TValue _patternValue;
};

template <typename TParameter, typename TObject>
struct ECFObjectMap: public ECFObject {
	std::map<TParameter, TObject> &value;

	ECFObjectMap(std::map<TParameter, TObject> &value): value(value) {}

	virtual ECFParameter* getParameter(const std::string &name)
	{
		if (ECFObject::getParameter(name) != NULL) {
			return ECFObject::getParameter(name);
		}
		TParameter key;
		if (ECFValueHolder<TParameter>(key).setValue(name)) {
			parameters.push_back(&value[key]);
			parameters.back()->name = name;
			parameters.back()->metadata = metadata.suffix(1);
			return parameters.back();
		} else {
			return NULL;
		}
	}

	virtual const ECFParameter* getPattern() const
	{
		return registerPatternParameter(new TObject());
	}
};

template <typename TParameter1, typename TParameter2, typename TValue>
struct ECFValueMapMap: public ECFObject {
	std::map<TParameter1, std::map<TParameter2, TValue> > &value;

	ECFValueMapMap(std::map<TParameter1, std::map<TParameter2, TValue> > &value): value(value) {}

	virtual ECFParameter* getParameter(const std::string &name)
	{
		if (ECFObject::getParameter(name) != NULL) {
			return ECFObject::getParameter(name);
		}
		TParameter1 key;
		if (ECFValueHolder<TParameter1>(key).setValue(name)) {
			return registerParameter(name, new ECFValueMap<TParameter2, TValue>(value[key]), metadata.suffix(1));
		} else {
			return NULL;
		}
	}

	virtual const ECFParameter* getPattern() const
	{
		return registerPatternParameter(new ECFValueMap<TParameter2, TValue>(_patternValue));
	}

private:
	mutable std::map<TParameter2, TValue> _patternValue;
};


template <typename TParameter1, typename TParameter2, typename TObject>
struct ECFObjectMapMap: public ECFObject {
	std::map<TParameter1, std::map<TParameter2, TObject> > &value;

	ECFObjectMapMap(std::map<TParameter1, std::map<TParameter2, TObject> > &value): value(value) {}

	virtual ECFParameter* getParameter(const std::string &name)
	{
		if (ECFObject::getParameter(name) != NULL) {
			return ECFObject::getParameter(name);
		}
		TParameter1 key;
		if (ECFValueHolder<TParameter1>(key).setValue(name)) {
			return registerParameter(name, new ECFObjectMap<TParameter2, TObject>(value[key]), metadata.suffix(1));
		} else {
			return NULL;
		}
	}

	virtual const ECFParameter* getPattern() const
	{
		return registerPatternParameter(new ECFObjectMap<TParameter2, TObject>(_patternValue));
	}

private:
	mutable std::map<TParameter2, TObject> _patternValue;
};

}

#endif /* SRC_CONFIG_OBJECTHOLDER_H_ */
