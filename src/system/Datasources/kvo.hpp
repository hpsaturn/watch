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

#ifndef ___LUNOKIOT__SYSTEM__KVO__BASE___
#define ___LUNOKIOT__SYSTEM__KVO__BASE___

#include <esp_event.h>
#include <esp_event_base.h>

#include <functional>

typedef std::function<void ()> KVOCallback;

class EventKVO {
    public:
        esp_event_handler_instance_t instance;
        KVOCallback callback;
        int event;
        EventKVO(KVOCallback callback, int SystemEvent);
        ~EventKVO();
};

#endif
