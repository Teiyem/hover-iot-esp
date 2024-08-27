#pragma once

class IotComponent
{
public:
    virtual void start() = 0;
    virtual void stop() = 0;
    bool started() const;
protected:
    bool _started = false;
};