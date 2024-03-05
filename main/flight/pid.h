#pragma once

#include "inttypes.h"

const char pidNames[] =
"ROLL;"
"PITCH;"
"YAW;"
"LEVEL;"
"MAG;";

typedef struct pidf_s {
    uint8_t P;
    uint8_t I;
    uint8_t D;
    uint16_t F;
} pidf_t;

#define XYZ_AXIS_COUNT 3
#define MAX_PROFILE_NAME_LENGTH 8u
#define PID_ITEM_COUNT 5u


typedef struct pidProfile_s {
    uint16_t yaw_lowpass_hz;                // Additional yaw filter when yaw axis too noisy
    uint16_t dterm_lpf1_static_hz;          // Static Dterm lowpass 1 filter cutoff value in hz
    uint16_t dterm_notch_hz;                // Biquad dterm notch hz
    uint16_t dterm_notch_cutoff;            // Biquad dterm notch low cutoff

    pidf_t  pid[PID_ITEM_COUNT];

    uint8_t dterm_lpf1_type;                // Filter type for dterm lowpass 1
    uint8_t itermWindupPointPercent;        // iterm windup threshold, percent motor saturation
    uint16_t pidSumLimit;
    uint16_t pidSumLimitYaw;
    uint8_t pidAtMinThrottle;               // Disable/Enable pids on zero throttle. Normally even without airmode P and D would be active.
    uint8_t angle_limit;                    // Max angle in degrees in Angle mode

    uint8_t horizon_limit_degrees;          // in Horizon mode, zero levelling when the quad's attitude exceeds this angle
    uint8_t horizon_ignore_sticks;          // 0 = default, meaning both stick and attitude attenuation; 1 = only attitude attenuation

    // Betaflight PID controller parameters
    uint8_t anti_gravity_gain;              // AntiGravity Gain (was itermAcceleratorGain)
    uint16_t yawRateAccelLimit;             // yaw accel limiter for deg/sec/ms
    uint16_t rateAccelLimit;                // accel limiter roll/pitch deg/sec/ms
    uint16_t crash_dthreshold;              // dterm crash value
    uint16_t crash_gthreshold;              // gyro crash value
    uint16_t crash_setpoint_threshold;      // setpoint must be below this value to detect crash, so flips and rolls are not interpreted as crashes
    uint16_t crash_time;                    // ms
    uint16_t crash_delay;                   // ms
    uint8_t crash_recovery_angle;           // degrees
    uint8_t crash_recovery_rate;            // degree/second
    uint16_t crash_limit_yaw;               // limits yaw errorRate, so crashes don't cause huge throttle increase
    uint16_t itermLimit;
    uint16_t dterm_lpf2_static_hz;          // Static Dterm lowpass 2 filter cutoff value in hz
    uint8_t crash_recovery;                 // off, on, on and beeps when it is in crash recovery mode
    uint8_t throttle_boost;                 // how much should throttle be boosted during transient changes 0-100, 100 adds 10x hpf filtered throttle
    uint8_t throttle_boost_cutoff;          // Which cutoff frequency to use for throttle boost. higher cutoffs keep the boost on for shorter. Specified in hz.
    uint8_t iterm_rotation;                 // rotates iterm to translate world errors to local coordinate system
    uint8_t iterm_relax_type;               // Specifies type of relax algorithm
    uint8_t iterm_relax_cutoff;             // This cutoff frequency specifies a low pass filter which predicts average response of the quad to setpoint
    uint8_t iterm_relax;                    // Enable iterm suppression during stick input
    uint8_t acro_trainer_angle_limit;       // Acro trainer roll/pitch angle limit in degrees
    uint8_t acro_trainer_debug_axis;        // The axis for which record debugging values are captured 0=roll, 1=pitch
    uint8_t acro_trainer_gain;              // The strength of the limiting. Raising may reduce overshoot but also lead to oscillation around the angle limit
    uint16_t acro_trainer_lookahead_ms;     // The lookahead window in milliseconds used to reduce overshoot
    uint8_t abs_control_gain;               // How strongly should the absolute accumulated error be corrected for
    uint8_t abs_control_limit;              // Limit to the correction
    uint8_t abs_control_error_limit;        // Limit to the accumulated error
    uint8_t abs_control_cutoff;             // Cutoff frequency for path estimation in abs control
    uint8_t dterm_lpf2_type;                // Filter type for 2nd dterm lowpass
    uint16_t dterm_lpf1_dyn_min_hz;         // Dterm lowpass filter 1 min hz when in dynamic mode
    uint16_t dterm_lpf1_dyn_max_hz;         // Dterm lowpass filter 1 max hz when in dynamic mode
    uint8_t launchControlMode;              // Whether launch control is limited to pitch only (launch stand or top-mount) or all axes (on battery)
    uint8_t launchControlThrottlePercent;   // Throttle percentage to trigger launch for launch control
    uint8_t launchControlAngleLimit;        // Optional launch control angle limit (requires ACC)
    uint8_t launchControlGain;              // Iterm gain used while launch control is active
    uint8_t launchControlAllowTriggerReset; // Controls trigger behavior and whether the trigger can be reset
    uint8_t use_integrated_yaw;             // Selects whether the yaw pidsum should integrated
    uint8_t integrated_yaw_relax;           // Specifies how much integrated yaw should be reduced to offset the drag based yaw component
    uint8_t thrustLinearization;            // Compensation factor for pid linearization
    uint8_t d_min[XYZ_AXIS_COUNT];          // Minimum D value on each axis
    uint8_t d_min_gain;                     // Gain factor for amount of gyro / setpoint activity required to boost D
    uint8_t d_min_advance;                  // Percentage multiplier for setpoint input to boost algorithm
    uint8_t motor_output_limit;             // Upper limit of the motor output (percent)
    int8_t auto_profile_cell_count;         // Cell count for this profile to be used with if auto PID profile switching is used
    uint8_t transient_throttle_limit;       // Maximum DC component of throttle change to mix into throttle to prevent airmode mirroring noise
    char profileName[MAX_PROFILE_NAME_LENGTH + 1]; // Descriptive name for profile

    uint8_t dyn_idle_min_rpm;               // minimum motor speed enforced by the dynamic idle controller
    uint8_t dyn_idle_p_gain;                // P gain during active control of rpm
    uint8_t dyn_idle_i_gain;                // I gain during active control of rpm
    uint8_t dyn_idle_d_gain;                // D gain for corrections around rapid changes in rpm
    uint8_t dyn_idle_max_increase;          // limit on maximum possible increase in motor idle drive during active control
    uint8_t dyn_idle_start_increase;        // limit on maximum possible increase in motor idle drive with airmode not activated

    uint8_t feedforward_transition;         // Feedforward attenuation around centre sticks
    uint8_t feedforward_averaging;          // Number of packets to average when averaging is on
    uint8_t feedforward_smooth_factor;      // Amount of lowpass type smoothing for feedforward steps
    uint8_t feedforward_jitter_factor;      // Number of RC steps below which to attenuate feedforward
    uint8_t feedforward_boost;              // amount of setpoint acceleration to add to feedforward, 10 means 100% added
    uint8_t feedforward_max_rate_limit;     // Maximum setpoint rate percentage for feedforward

    uint8_t dterm_lpf1_dyn_expo;            // set the curve for dynamic dterm lowpass filter
    uint8_t level_race_mode;                // NFE race mode - when true pitch setpoint calculation is gyro based in level mode
    uint8_t vbat_sag_compensation;          // Reduce motor output by this percentage of the maximum compensation amount

    uint8_t simplified_pids_mode;
    uint8_t simplified_master_multiplier;
    uint8_t simplified_roll_pitch_ratio;
    uint8_t simplified_i_gain;
    uint8_t simplified_d_gain;
    uint8_t simplified_pi_gain;
    uint8_t simplified_dmin_ratio;
    uint8_t simplified_feedforward_gain;
    uint8_t simplified_dterm_filter;
    uint8_t simplified_dterm_filter_multiplier;
    uint8_t simplified_pitch_pi_gain;

    uint8_t anti_gravity_cutoff_hz;
    uint8_t anti_gravity_p_gain;
    uint8_t tpa_mode;                       // Controls which PID terms TPA effects
    uint8_t tpa_rate;                       // Percent reduction in P or D at full throttle
    uint16_t tpa_breakpoint;                // Breakpoint where TPA is activated

    uint8_t angle_feedforward_smoothing_ms; // Smoothing factor for angle feedforward as time constant in milliseconds
    uint8_t angle_earth_ref;                // Control amount of "co-ordination" from yaw into roll while pitched forward in angle mode
    uint16_t horizon_delay_ms;              // delay when Horizon Strength increases, 50 = 500ms time constant
    uint8_t tpa_low_rate;                   // Percent reduction in P or D at zero throttle
    uint16_t tpa_low_breakpoint;            // Breakpoint where lower TPA is deactivated
    uint8_t tpa_low_always;                 // off, on - if OFF then low TPA is only active until tpa_low_breakpoint is reached the first time

    uint8_t ez_landing_threshold;           // Threshold stick position below which motor output is limited
    uint8_t ez_landing_limit;               // Maximum motor output when all sticks centred and throttle zero
} pidProfile_t;
