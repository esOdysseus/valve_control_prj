/*
 * sys_sigslot_base.h
 *
 *  Created on: Mar 24, 2020
 *      Author: eunseok
 */

#ifndef _SYS_SIG_SLOT_BASE_H_
#define _SYS_SIG_SLOT_BASE_H_


#include <stdexcept>
#include <functional>
#include <list>
#include <memory>
#include <csignal>
#include <iostream>
#include <cassert>

#include <errno.h>

using std::runtime_error;

namespace sys_sigslot {

class SignalException : public runtime_error
{
public:
   SignalException(const std::string& _message)
	  : std::runtime_error(_message)
   {}
};


template <typename CHD_TYPE>
class ISlot
{
private:
	using Slot_Type = std::function<void(void)>;

public:
	static std::shared_ptr<CHD_TYPE> get_instance(void) {
		static std::shared_ptr<CHD_TYPE> _instance_ = std::shared_ptr<CHD_TYPE>(new CHD_TYPE());
		return _instance_;
	}

	~ISlot(void) {
		clear();
	}

	bool get_signal() {
		return _mbool_sig_occur_;
	}

protected:
	ISlot(void) {
		clear();
		connect_default_slot();
	}

	template<typename _Callable, typename... _Args>
	void _connect_slot_(_Callable&& __f, _Args&&... __args)
	{
		int sig_num = CHD_TYPE::SIG_NUMBER;
		Slot_Type func = std::bind(std::forward<_Callable>(__f), std::forward<_Args>(__args)... , sig_num);
		_mlist_func_p_.push_back(func);
	}

private:


	void clear(void) {
		_mbool_sig_occur_ = false;
		_mlist_func_p_.clear();
	}

	static void default_slot(int __ignore_arg) {
		std::cout << "default_slot() is called." << std::endl;
		std::shared_ptr<CHD_TYPE> instance = CHD_TYPE::get_instance();

		std::cout << "default_slot() call sub-sequenced call-back func." << std::endl;
		instance->_mbool_sig_occur_ = true;
		for(auto itor=instance->_mlist_func_p_.begin(); itor != instance->_mlist_func_p_.end(); itor++) {
			(*itor)();
		}
	}

	void connect_default_slot(void) {
		assert(CHD_TYPE::SIG_NUMBER > 0);
		if(std::signal(CHD_TYPE::SIG_NUMBER, ISlot<CHD_TYPE>::default_slot) == SIG_ERR)
		{
			throw SignalException("[Error] connect_slot_direct(): 'signal' function is failed.");
		}
	}

protected:
	sig_atomic_t _mbool_sig_occur_;

	std::list<Slot_Type> _mlist_func_p_;

};

}   // sys_sigslot

#endif /* _SYS_SIG_SLOT_BASE_H_ */
