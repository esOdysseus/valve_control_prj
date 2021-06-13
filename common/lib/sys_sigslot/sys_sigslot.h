/*
 * sys_sigslot.h
 *
 *  Created on: Mar 24, 2020
 *      Author: eunseok
 */

#ifndef SYS_SIGSLOT_H_
#define SYS_SIGSLOT_H_


#include "sys_sigslot_base.h"

namespace sys_sigslot {

/** Signal-Slot : Ctrl+C signal(2) */
class CExitSig : public ISlot<CExitSig> {
public:
	static constexpr int SIG_NUMBER = (int)SIGINT;

public:
	CExitSig(void) = default;

	template<typename _Callable, typename... _Args>
	void connect(_Callable&& __f, _Args&&... __args)
	{
		_connect_slot_(std::forward<_Callable>(__f), std::forward<_Args>(__args)...);
	}

	int emit(int var) {
		return std::raise(SIG_NUMBER);
	}

};


/** Signal-Slot : USER signal(10) */
class CUser01Sig : public ISlot<CUser01Sig> {
public:
	static constexpr int SIG_NUMBER = (int)SIGUSR1;

public:
	CUser01Sig(void): ISlot<CUser01Sig>() {
		_m_var_ = 0;
	}

	template<typename _Callable, typename... _Args>
	void connect(_Callable&& __f, _Args&&... __args)
	{
		_connect_slot_(std::forward<_Callable>(__f), std::forward<_Args>(__args)... , std::ref(_m_var_));
	}

	int emit(int& var) {
		_m_var_ = var;
		return std::raise(SIG_NUMBER);
	}

private:
	int _m_var_;

};

}   // sys_sigslot


#endif /* SYS_SIGSLOT_H_ */
