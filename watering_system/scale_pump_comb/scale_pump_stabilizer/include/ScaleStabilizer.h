#pragma once

/**
 * @class ScaleStablilizer
 * @brief Uses a simple exponential moving average (EMA) 
 * combined with a small deadband for the displayed weight.
 * 
 * y_k = a*x_k + (1-a)*y_(k-1)
 */
class ScaleStabilizer {
private:
    float _alpha;
    float _filteredValue;
    bool _initialized;

public:
    ScaleStabilizer();

    /**
     * @brief Initilize stabilizer.
     * @param alpha Sensitivity. 0.1: low. 0.5: high.
     */
    void begin(float alpha);

    /**
     * @brief Reset the stabilizer. 
     * Should be done for every tare()
     */
    void reset();

    /**
     * @brief Add a reading to the stabilizer.
     * @param reading Raw unit value of scale.
     */
    void add(float reading);

    /**
     * @brief Returns the stabilized scale value.
     */
    float getValue();
};