#pragma once

#include "esp_err.h"

/**
 * A base class for core components.
 */
class IotComponent
{
public:
    virtual esp_err_t start() = 0;
    virtual void stop() = 0;
    bool started() const;
protected:
    bool _started = false; /**< Whether the component is started or not. */
};