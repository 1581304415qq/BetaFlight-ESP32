import numpy as np
import math

class MadgwickAHRS:
    """
    Implementation of Madgwick's IMU and AHRS algorithms in Python.
    See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
    
    Original C implementation by Sebastian Madgwick
    Python implementation converted from C code
    """
    
    def __init__(self, sample_freq=512.0, beta=0.1):
        """
        Initialize the AHRS algorithm.
        
        Args:
            sample_freq: Sample frequency in Hz
            beta: 2 * proportional gain (Kp)
        """
        self.sample_freq = sample_freq
        self.beta = beta
        
        # Quaternion of sensor frame relative to auxiliary frame
        self.q0 = 1.0
        self.q1 = 0.0
        self.q2 = 0.0
        self.q3 = 0.0
    
    def update(self, gx, gy, gz, ax, ay, az, mx, my, mz):
        """
        Madgwick AHRS algorithm update with magnetometer.
        
        Args:
            gx, gy, gz: Gyroscope measurements in rad/s
            ax, ay, az: Accelerometer measurements in any consistent unit
            mx, my, mz: Magnetometer measurements in any consistent unit
        """
        # Use IMU algorithm if magnetometer measurement invalid
        if (mx == 0.0) and (my == 0.0) and (mz == 0.0):
            self.update_imu(gx, gy, gz, ax, ay, az)
            return
        
        # Rate of change of quaternion from gyroscope
        q_dot1 = 0.5 * (-self.q1 * gx - self.q2 * gy - self.q3 * gz)
        q_dot2 = 0.5 * (self.q0 * gx + self.q2 * gz - self.q3 * gy)
        q_dot3 = 0.5 * (self.q0 * gy - self.q1 * gz + self.q3 * gx)
        q_dot4 = 0.5 * (self.q0 * gz + self.q1 * gy - self.q2 * gx)
        
        # Compute feedback only if accelerometer measurement valid
        if not ((ax == 0.0) and (ay == 0.0) and (az == 0.0)):
            # Normalize accelerometer measurement
            recip_norm = self.inv_sqrt(ax * ax + ay * ay + az * az)
            ax *= recip_norm
            ay *= recip_norm
            az *= recip_norm
            
            # Normalize magnetometer measurement
            recip_norm = self.inv_sqrt(mx * mx + my * my + mz * mz)
            mx *= recip_norm
            my *= recip_norm
            mz *= recip_norm
            
            # Auxiliary variables to avoid repeated arithmetic
            _2q0mx = 2.0 * self.q0 * mx
            _2q0my = 2.0 * self.q0 * my
            _2q0mz = 2.0 * self.q0 * mz
            _2q1mx = 2.0 * self.q1 * mx
            _2q0 = 2.0 * self.q0
            _2q1 = 2.0 * self.q1
            _2q2 = 2.0 * self.q2
            _2q3 = 2.0 * self.q3
            _2q0q2 = 2.0 * self.q0 * self.q2
            _2q2q3 = 2.0 * self.q2 * self.q3
            q0q0 = self.q0 * self.q0
            q0q1 = self.q0 * self.q1
            q0q2 = self.q0 * self.q2
            q0q3 = self.q0 * self.q3
            q1q1 = self.q1 * self.q1
            q1q2 = self.q1 * self.q2
            q1q3 = self.q1 * self.q3
            q2q2 = self.q2 * self.q2
            q2q3 = self.q2 * self.q3
            q3q3 = self.q3 * self.q3
            
            # Reference direction of Earth's magnetic field
            hx = mx * q0q0 - _2q0my * self.q3 + _2q0mz * self.q2 + mx * q1q1 + _2q1 * my * self.q2 + _2q1 * mz * self.q3 - mx * q2q2 - mx * q3q3
            hy = _2q0mx * self.q3 + my * q0q0 - _2q0mz * self.q1 + _2q1mx * self.q2 - my * q1q1 + my * q2q2 + _2q2 * mz * self.q3 - my * q3q3
            _2bx = math.sqrt(hx * hx + hy * hy)
            _2bz = -_2q0mx * self.q2 + _2q0my * self.q1 + mz * q0q0 + _2q1mx * self.q3 - mz * q1q1 + _2q2 * my * self.q3 - mz * q2q2 + mz * q3q3
            _4bx = 2.0 * _2bx
            _4bz = 2.0 * _2bz
            
            # Gradient decent algorithm corrective step
            s0 = -_2q2 * (2.0 * q1q3 - _2q0q2 - ax) + _2q1 * (2.0 * q0q1 + _2q2q3 - ay) - _2bz * self.q2 * (_2bx * (0.5 - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * self.q3 + _2bz * self.q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * self.q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5 - q1q1 - q2q2) - mz)
            s1 = _2q3 * (2.0 * q1q3 - _2q0q2 - ax) + _2q0 * (2.0 * q0q1 + _2q2q3 - ay) - 4.0 * self.q1 * (1 - 2.0 * q1q1 - 2.0 * q2q2 - az) + _2bz * self.q3 * (_2bx * (0.5 - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * self.q2 + _2bz * self.q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * self.q3 - _4bz * self.q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5 - q1q1 - q2q2) - mz)
            s2 = -_2q0 * (2.0 * q1q3 - _2q0q2 - ax) + _2q3 * (2.0 * q0q1 + _2q2q3 - ay) - 4.0 * self.q2 * (1 - 2.0 * q1q1 - 2.0 * q2q2 - az) + (-_4bx * self.q2 - _2bz * self.q0) * (_2bx * (0.5 - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * self.q1 + _2bz * self.q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * self.q0 - _4bz * self.q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5 - q1q1 - q2q2) - mz)
            s3 = _2q1 * (2.0 * q1q3 - _2q0q2 - ax) + _2q2 * (2.0 * q0q1 + _2q2q3 - ay) + (-_4bx * self.q3 + _2bz * self.q1) * (_2bx * (0.5 - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * self.q0 + _2bz * self.q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * self.q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5 - q1q1 - q2q2) - mz)
            
            # Normalize step magnitude
            recip_norm = self.inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3)
            s0 *= recip_norm
            s1 *= recip_norm
            s2 *= recip_norm
            s3 *= recip_norm
            
            # Apply feedback step
            q_dot1 -= self.beta * s0
            q_dot2 -= self.beta * s1
            q_dot3 -= self.beta * s2
            q_dot4 -= self.beta * s3
        
        # Integrate rate of change of quaternion to yield quaternion
        self.q0 += q_dot1 * (1.0 / self.sample_freq)
        self.q1 += q_dot2 * (1.0 / self.sample_freq)
        self.q2 += q_dot3 * (1.0 / self.sample_freq)
        self.q3 += q_dot4 * (1.0 / self.sample_freq)
        
        # Normalize quaternion
        recip_norm = self.inv_sqrt(self.q0 * self.q0 + self.q1 * self.q1 + self.q2 * self.q2 + self.q3 * self.q3)
        self.q0 *= recip_norm
        self.q1 *= recip_norm
        self.q2 *= recip_norm
        self.q3 *= recip_norm
    
    def update_imu(self, gx, gy, gz, ax, ay, az):
        """
        Madgwick IMU algorithm update (no magnetometer).
        
        Args:
            gx, gy, gz: Gyroscope measurements in rad/s
            ax, ay, az: Accelerometer measurements in any consistent unit
        """
        # Rate of change of quaternion from gyroscope
        q_dot1 = 0.5 * (-self.q1 * gx - self.q2 * gy - self.q3 * gz)
        q_dot2 = 0.5 * (self.q0 * gx + self.q2 * gz - self.q3 * gy)
        q_dot3 = 0.5 * (self.q0 * gy - self.q1 * gz + self.q3 * gx)
        q_dot4 = 0.5 * (self.q0 * gz + self.q1 * gy - self.q2 * gx)
        
        # Compute feedback only if accelerometer measurement valid
        if not ((ax == 0.0) and (ay == 0.0) and (az == 0.0)):
            # Normalize accelerometer measurement
            recip_norm = self.inv_sqrt(ax * ax + ay * ay + az * az)
            ax *= recip_norm
            ay *= recip_norm
            az *= recip_norm
            
            # Auxiliary variables to avoid repeated arithmetic
            _2q0 = 2.0 * self.q0
            _2q1 = 2.0 * self.q1
            _2q2 = 2.0 * self.q2
            _2q3 = 2.0 * self.q3
            _4q0 = 4.0 * self.q0
            _4q1 = 4.0 * self.q1
            _4q2 = 4.0 * self.q2
            _8q1 = 8.0 * self.q1
            _8q2 = 8.0 * self.q2
            q0q0 = self.q0 * self.q0
            q1q1 = self.q1 * self.q1
            q2q2 = self.q2 * self.q2
            q3q3 = self.q3 * self.q3
            
            # Gradient decent algorithm corrective step
            s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay
            s1 = _4q1 * q3q3 - _2q3 * ax + 4.0 * q0q0 * self.q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az
            s2 = 4.0 * q0q0 * self.q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az
            s3 = 4.0 * q1q1 * self.q3 - _2q1 * ax + 4.0 * q2q2 * self.q3 - _2q2 * ay
            
            # Normalize step magnitude
            recip_norm = self.inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3)
            s0 *= recip_norm
            s1 *= recip_norm
            s2 *= recip_norm
            s3 *= recip_norm
            
            # Apply feedback step
            q_dot1 -= self.beta * s0
            q_dot2 -= self.beta * s1
            q_dot3 -= self.beta * s2
            q_dot4 -= self.beta * s3
        
        # Integrate rate of change of quaternion to yield quaternion
        self.q0 += q_dot1 * (1.0 / self.sample_freq)
        self.q1 += q_dot2 * (1.0 / self.sample_freq)
        self.q2 += q_dot3 * (1.0 / self.sample_freq)
        self.q3 += q_dot4 * (1.0 / self.sample_freq)
        
        # Normalize quaternion
        recip_norm = self.inv_sqrt(self.q0 * self.q0 + self.q1 * self.q1 + self.q2 * self.q2 + self.q3 * self.q3)
        self.q0 *= recip_norm
        self.q1 *= recip_norm
        self.q2 *= recip_norm
        self.q3 *= recip_norm
    
    def inv_sqrt(self, x):
        """
        Fast inverse square-root
        See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
        
        Args:
            x: Input value
            
        Returns:
            Inverse square root of x
        """
        # For Python, we use the numpy version for accuracy and portability
        # rather than the bit manipulation trick from the C version
        return 1.0 / math.sqrt(x)
    
    def get_quaternion(self):
        """
        Get the current orientation quaternion.
        
        Returns:
            tuple: (q0, q1, q2, q3) representing the orientation quaternion
        """
        return (self.q0, self.q1, self.q2, self.q3)
    
    def get_euler_angles(self):
        """
        Convert quaternion to Euler angles (roll, pitch, yaw).
        
        Returns:
            tuple: (roll, pitch, yaw) in radians
        """
        # Roll (x-axis rotation)
        sinr_cosp = 2.0 * (self.q0 * self.q1 + self.q2 * self.q3)
        cosr_cosp = 1.0 - 2.0 * (self.q1 * self.q1 + self.q2 * self.q2)
        roll = math.atan2(sinr_cosp, cosr_cosp)
        
        # Pitch (y-axis rotation)
        sinp = 2.0 * (self.q0 * self.q2 - self.q3 * self.q1)
        if abs(sinp) >= 1:
            pitch = math.copysign(math.pi / 2, sinp)  # Use 90 degrees if out of range
        else:
            pitch = math.asin(sinp)
        
        # Yaw (z-axis rotation)
        siny_cosp = 2.0 * (self.q0 * self.q3 + self.q1 * self.q2)
        cosy_cosp = 1.0 - 2.0 * (self.q2 * self.q2 + self.q3 * self.q3)
        yaw = math.atan2(siny_cosp, cosy_cosp)
        
        return (roll, pitch, yaw)


# Example usage
if __name__ == "__main__":
    # Create AHRS object with default settings
    ahrs = MadgwickAHRS(sample_freq=100.0, beta=0.1)
    
    # Example sensor data (you would replace this with your actual sensor readings)
    gx, gy, gz = 0.1, 0.2, 0.3  # gyroscope data in rad/s
    ax, ay, az = 0.0, 0.0, 1.0  # accelerometer data (normalized)
    mx, my, mz = 0.5, 0.0, 0.5  # magnetometer data (normalized)
    
    # Update the filter with sensor measurements
    ahrs.update(gx, gy, gz, ax, ay, az, mx, my, mz)
    
    # Get the current orientation as a quaternion
    q0, q1, q2, q3 = ahrs.get_quaternion()
    print(f"Quaternion: ({q0:.4f}, {q1:.4f}, {q2:.4f}, {q3:.4f})")
    
    # Get the current orientation as Euler angles
    roll, pitch, yaw = ahrs.get_euler_angles()
    print(f"Euler angles (rad): Roll: {roll:.4f}, Pitch: {pitch:.4f}, Yaw: {yaw:.4f}")
    print(f"Euler angles (deg): Roll: {math.degrees(roll):.4f}, Pitch: {math.degrees(pitch):.4f}, Yaw: {math.degrees(yaw):.4f}")