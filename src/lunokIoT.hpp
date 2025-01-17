//
//    LunokWatch, a open source smartwatch software
//    Copyright (C) 2022,2023  Jordi Rubió <jordi@binarycell.org>
//    This file is part of LunokWatch.
//
// LunokWatch is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software 
// Foundation, either version 3 of the License, or (at your option) any later 
// version.
//
// LunokWatch is distributed in the hope that it will be useful, but WITHOUT 
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
// details.
//
// You should have received a copy of the GNU General Public License along with 
// LunokWatch. If not, see <https://www.gnu.org/licenses/>. 
//

#ifndef ___lunokIoT___H____
#define ___lunokIoT___H____

#include "lunokiot_config.hpp" // WARNING: generated file, the original from "tools/lunokiot_config.hpp.template"
#include "system/Network/BLE.hpp"

// almost empty defines to clarify the paramethers direction (see declarations below)
#define IN const // clarify to developer if the data is input or output
#define OUT
#define INOUT
// other convenient syntactic-sugar
#define NO_CALLBACK nullptr
#define IGNORE_PARAM void *unused

// LoT is all you need for use it! ;)
#define LoT LunokIoT::Get

// yep... a f*kng singleton, you know C++11 stylish
class LunokIoT {
    public:
        static LunokIoT& Get() {
            static LunokIoT _myInstance;
            return _myInstance;
        }
        void ListSPIFFS();
        bool IsSPIFFSEnabled();
        bool IsNVSEnabled();
        void BootReason();
        bool IsNetworkInUse();
        bool CpuSpeed(IN uint32_t mhz);

        // delete copy and move constructors and assign operators
        LunokIoT(LunokIoT const&) = delete;             // Copy construct
        LunokIoT(LunokIoT&&) = delete;                  // Move construct
        LunokIoT& operator=(LunokIoT const&) = delete;  // Copy assign
        LunokIoT& operator=(LunokIoT &&) = delete;      // Move assign
        bool SPIFFSReady=false; // @TODO
        size_t selectedWatchFace=0;
    protected:
        LunokIoT();
        ~LunokIoT() {}; // implemented here ¿what kind of singleton.... x'D
    private:
        bool normalBoot=true;
        bool NVSReady=false;
        void InitLogs();
};

#endif
