#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>
#include <chrono>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

class Cafeteria {
    struct OrderState {
        std::shared_ptr<Bread> bread;
        std::shared_ptr<Sausage> sausage;
        HotDogHandler handler;
        net::strand<net::io_context::executor_type> strand;
        net::steady_timer bread_timer;
        net::steady_timer sausage_timer;
        bool bread_ready = false;
        bool sausage_ready = false;
        
        OrderState(net::io_context& io, 
                  std::shared_ptr<Bread> b, 
                  std::shared_ptr<Sausage> s,
                  HotDogHandler h,
                  net::strand<net::io_context::executor_type> str)
            : bread(std::move(b))
            , sausage(std::move(s))
            , handler(std::move(h))
            , strand(std::move(str))
            , bread_timer(io, std::chrono::milliseconds(1000))
            , sausage_timer(io, std::chrono::milliseconds(1500)) {
        }
    };

public:
    explicit Cafeteria(net::io_context& io)
        : io_{io}
        , work_(net::make_work_guard(io)) {
        // Таймер для сброса work_guard через 10 секунд,
        // чтобы io_context мог завершиться после приготовления всех заказов
        auto shutdown_timer = std::make_shared<net::steady_timer>(io_, std::chrono::seconds(10));
        shutdown_timer->async_wait([this, shutdown_timer](boost::system::error_code) {
            work_.reset();
        });
    }

    void OrderHotDog(HotDogHandler handler) {
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        
        auto strand = net::make_strand(io_);
        auto state = std::make_shared<OrderState>(io_, std::move(bread), std::move(sausage), 
                                                   std::move(handler), strand);
        
        auto gas_cooker = gas_cooker_;
        
        state->bread_timer.async_wait(
            net::bind_executor(state->strand, 
            [state, gas_cooker](boost::system::error_code ec) {
                if (!ec) {
                    state->bread->StartBake(*gas_cooker, [state]() {
                        state->bread_ready = true;
                        CheckAndAssemble(state);
                    });
                }
            }));
        
        state->sausage_timer.async_wait(
            net::bind_executor(state->strand,
            [state, gas_cooker](boost::system::error_code ec) {
                if (!ec) {
                    state->sausage->StartFry(*gas_cooker, [state]() {
                        state->sausage_ready = true;
                        CheckAndAssemble(state);
                    });
                }
            }));
    }

private:
    static void CheckAndAssemble(std::shared_ptr<OrderState> state) {
        if (state->bread_ready && state->sausage_ready) {
            try {
                state->bread->StopBaking();
                state->sausage->StopFry();
                
                HotDog hot_dog(state->bread->GetId() * 1000 + state->sausage->GetId(), 
                              state->sausage, state->bread);
                state->handler(Result<HotDog>(std::move(hot_dog)));
            } catch (...) {
                state->handler(Result<HotDog>::FromCurrentException());
            }
        }
    }

    net::io_context& io_;
    net::executor_work_guard<net::io_context::executor_type> work_;
    Store store_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};
