#pragma once

#include <cxxabi.h>

#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "Processor.h"

namespace whale {
namespace vision {
template <typename... Targs>
class ProcessorFactory {
 public:
    static ProcessorFactory* Instance() {
        if (nullptr == processor_Factory_) {
            processor_Factory_ = new ProcessorFactory();
        }
        return (processor_Factory_);
    }

    virtual ~ProcessorFactory(){};

    bool Regist(const std::string& strTypeName,
                std::function<Processor*(Targs&&... args)> pFunc) {
        if (nullptr == pFunc) {
            return (false);
        }
        std::string strRealTypeName = strTypeName;

        bool bReg =
            create_func_map_.insert(std::make_pair(strRealTypeName, pFunc))
                .second;
        return (bReg);
    }

    Processor* Create(const std::string& strTypeName, Targs&&... args) {
        auto iter = create_func_map_.find(strTypeName);
        if (iter == create_func_map_.end()) {
            return (nullptr);
        } else {
            return (iter->second(std::forward<Targs>(args)...));
        }
    }

 private:
    ProcessorFactory(){};
    static ProcessorFactory<Targs...>* processor_Factory_;
    std::unordered_map<std::string, std::function<Processor*(Targs&&...)> >
        create_func_map_;
};

template <typename... Targs>
ProcessorFactory<Targs...>* ProcessorFactory<Targs...>::processor_Factory_ =
    nullptr;

template <typename T, typename... Targs>
class DynamicCreator {
 public:
    struct Register {
        Register() {
            char* szDemangleName = nullptr;
            std::string strTypeName;
#ifdef __GNUC__
            szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr,
                                                 nullptr, nullptr);
#else
            seDemangleName = typeid(T).name();
#endif
            if (nullptr != szDemangleName) {
                strTypeName = szDemangleName;
                free(szDemangleName);
            }

            ProcessorFactory<Targs...>::Instance()->Regist(strTypeName,
                                                           CreateObject);
        }

        inline void doNothing() const {};
    };

    DynamicCreator() { register_.doNothing(); }
    virtual ~DynamicCreator() { register_.doNothing(); };

    static T* CreateObject(Targs&&... args) {
        return new T(std::forward<Targs>(args)...);
    }

    static Register register_;
};

template <typename T, typename... Targs>
typename DynamicCreator<T, Targs...>::Register
    DynamicCreator<T, Targs...>::register_;

class ProcessorBuild {
 public:
    template <typename... Targs>
    static Processor* CreateProcessor(const std::string& strTypeName,
                                      Targs&&... args) {
        Processor* p = ProcessorFactory<Targs...>::Instance()->Create(
            strTypeName, std::forward<Targs>(args)...);
        return (p);
    }

    template <typename... Targs>
    static std::shared_ptr<Processor> MakeSharedProcessor(
        const std::string& strTypeName, Targs&&... args) {
        std::shared_ptr<Processor> ptr(
            CreateProcessor(strTypeName, std::forward<Targs>(args)...));
        return ptr;
    }
};
}  // namespace vision
}  // namespace whale