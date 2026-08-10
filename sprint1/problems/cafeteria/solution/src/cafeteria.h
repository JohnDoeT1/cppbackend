#pragma once

#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>

#include <memory>
#include <functional>


#include "hotdog.h"
#include "result.h"


namespace net = boost::asio;
namespace sys = boost::system;


using HotDogHandler =
    std::function<void(Result<HotDog>)>;



class Cafeteria {

public:

    explicit Cafeteria(net::io_context& io)
        : io_{io}
    {
    }



    void OrderHotDog(HotDogHandler handler) {


        auto order =
            std::make_shared<Order>(
                io_,
                gas_cooker_,
                store_.GetSausage(),
                store_.GetBread(),
                std::move(handler)
            );


        order->Start();
    }



private:


    class Order :
        public std::enable_shared_from_this<Order> {


    public:

        Order(
            net::io_context& io,
            std::shared_ptr<GasCooker> cooker,
            std::shared_ptr<Sausage> sausage,
            std::shared_ptr<Bread> bread,
            HotDogHandler handler
        )
            :
            io_{io}
            , strand_{net::make_strand(io)}
            , cooker_{std::move(cooker)}
            , sausage_{std::move(sausage)}
            , bread_{std::move(bread)}
            , handler_{std::move(handler)}
            , sausage_timer_{io}
            , bread_timer_{io}
        {
        }



void Start() {

    auto self = shared_from_this();


    sausage_->StartFry(
        *cooker_,
        [self] {

            // Не берём максимум 2000 мс.
            // Оставляем запас, чтобы фактическое время
            // не вышло за пределы HotDog.
            self->sausage_timer_.expires_after(
                Milliseconds{1500}
            );


            self->sausage_timer_.async_wait(
                net::bind_executor(
                    self->strand_,
                    [self](sys::error_code ec) {

                        if (ec) {
                            return;
                        }

                        self->OnSausageReady();
                    }
                )
            );
        }
    );



    bread_->StartBake(
        *cooker_,
        [self] {

            self->bread_timer_.expires_after(
                Milliseconds{1000}
            );


            self->bread_timer_.async_wait(
                net::bind_executor(
                    self->strand_,
                    [self](sys::error_code ec) {

                        if (ec) {
                            return;
                        }

                        self->OnBreadReady();
                    }
                )
            );
        }
    );
}



    private:


        void OnSausageReady() {

            sausage_->StopFry();

            sausage_ready_ = true;

            TryFinish();
        }



        void OnBreadReady() {

            bread_->StopBaking();

            bread_ready_ = true;

            TryFinish();
        }



void TryFinish() {

    if (!sausage_ready_
        || !bread_ready_) {

        return;
    }


    try {

        handler_(
            HotDog{
                ++next_id_,
                sausage_,
                bread_
            }
        );

    } catch (...) {

        handler_(
            Result<HotDog>::FromCurrentException()
        );
    }
}



    private:


        net::io_context& io_;


        net::strand<
            net::io_context::executor_type
        > strand_;


        std::shared_ptr<GasCooker> cooker_;

        std::shared_ptr<Sausage> sausage_;

        std::shared_ptr<Bread> bread_;


        HotDogHandler handler_;


        net::steady_timer sausage_timer_;

        net::steady_timer bread_timer_;


        bool sausage_ready_ = false;

        bool bread_ready_ = false;


        inline static int next_id_ = 0;
    };



private:

    net::io_context& io_;


    Store store_;


    std::shared_ptr<GasCooker> gas_cooker_ =
        std::make_shared<GasCooker>(io_);
};
