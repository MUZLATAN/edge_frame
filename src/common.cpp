#include "common.h"

namespace whale {
namespace vision {

GlobalVariable* GetGlobalVariable(void) {
    static GlobalVariable t;
    return &t;
};

}  // namespace vision
}  // namespace whale