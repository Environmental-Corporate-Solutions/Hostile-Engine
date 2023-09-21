/******************************************************************************
@file       Delegate.h
@author     Sam Biks
@par        DP email: s.biks@digipen.edu
@par        Course: GAM 250

@brief

	Copyright ?2022 DigiPen (USA) Corporation.
******************************************************************************/
#pragma once
#include <memory>

template <typename FuncParam>
class Delegate
{
	std::weak_ptr<void> m_Object;
	typedef bool (*DelegateFunction)(std::weak_ptr<void>, FuncParam);
	DelegateFunction m_Function;

	template <class T, void(T::* Func)(FuncParam)>
	static bool CreateDelegateFunction(std::weak_ptr<void> _Object, FuncParam _Param)
	{
		bool bad = false;
		if (_Object.expired())
		{
			bad = true;
		}
		else
		{
			std::shared_ptr<T> temp = std::static_pointer_cast<T>(_Object.lock());
			(temp.get()->*Func)(_Param);
		}

		return bad;
	}

public:
	template <class T, void(T::* Func)(FuncParam)>
	static Delegate CreateDelegate(std::weak_ptr<T> _Object)
	{
		Delegate test;
		test.m_Object = _Object;
		test.m_Function = &CreateDelegateFunction<T, Func>;
		return test;
	}

	bool operator()(const FuncParam _Param)	const
	{
		return (*m_Function)(m_Object, _Param);
	}

	bool operator==(const Delegate& _Rhs) const
	{
		return (m_Object.lock() == _Rhs.m_Object.lock());
	}
};