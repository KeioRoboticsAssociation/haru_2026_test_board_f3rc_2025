#pragma once

#include "UartLink.hpp"

// RS485Linkクラス。UartLinkのエイリアス
using RS485Link = UartLink;

// Publisher
template<typename... Args>
using RS485LinkPublisher = Publisher<RS485Link, uint8_t, Args...>;

// Subscriber
template<typename... Args>
class RS485LinkSubscriber : public Subscriber<RS485Link, uint8_t, Args...> {
    public:
        RS485LinkSubscriber(RS485Link& communication, uint8_t frame_id) : Subscriber<RS485Link, uint8_t, Args...>(communication, frame_id) {};

        void set_callback(std::function<void(Args...)> callback){
            this->callback = [callback](uint8_t id, Args... args){
                if (id == this->communication.device_id) {
                    callback(args...);
                }
            };
        }
};

