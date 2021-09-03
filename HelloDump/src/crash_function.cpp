#include "crash_function.h"
#include <iostream>
#include <vector>

namespace crash_function {
    void MemoryAccessCrash()
    {
        std::cout << "Normal null pointer crash" << std::endl;

        char* p = 0;
        *p = 88;
    }

    void OutOfBoundsVectorCrash()
    {
        std::cout << "std::vector out of bounds crash!" << std::endl;

        std::vector<int> v;
        v[0] = 88;
    }

    void AbortCrash()
    {
        std::cout << "Calling Abort" << std::endl;
        abort();
    }

    void VirtualFunctionCallCrash()
    {
        struct B
        {
            B()
            {
                std::cout << "Pure Virtual Function Call crash!" << std::endl;
                Bar();
            }

            virtual void Foo() = 0;

            void Bar()
            {
                Foo();
            }
        };

        struct D : public B
        {
            void Foo()
            {
            }
        };

        B* b = new D;
        // Just to silence the warning C4101: 'VirtualFunctionCallCrash::B::Foo' : unreferenced local variable
        b->Foo();
    }
};
