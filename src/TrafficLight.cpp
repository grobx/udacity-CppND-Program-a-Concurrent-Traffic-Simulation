#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // wait for and return new messages from the queue
    std::unique_lock<std::mutex> lck(_mtx);
    _condition.wait(lck, [this] { return !_queue.empty(); });
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // add a new message to the queue and send a notification.
    std::lock_guard<std::mutex> lck(_mtx);
    _queue.emplace_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while(_messageQueue.receive() != TrafficLightPhase::green);
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    using namespace std::chrono_literals;

    // take the first point in time
    auto lastUpdate = std::chrono::system_clock::now();

    // pick number of seconds at random between 4s and 6s
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4000, 6000);
    auto cycle_duration = std::chrono::milliseconds(distr(eng));

    for(;;) {
        // take a break
        std::this_thread::sleep_for(1ms);

        // measure time between two loop cycles
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - lastUpdate);

        // after reaching cycle duration
        if (delta >= cycle_duration) {
            // compute next phase
            TrafficLightPhase newPhase = _currentPhase == TrafficLightPhase::red ?
                        TrafficLightPhase::green : TrafficLightPhase::red;

            // save as current phase
            _currentPhase = newPhase;

            // send new phase update to message queue
            _messageQueue.send(std::move(newPhase));

            // update the end point in time
            lastUpdate = std::chrono::system_clock::now();
        }
    }
}
