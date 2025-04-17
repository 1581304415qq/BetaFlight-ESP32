import numpy as np
import math

RAD2DEG = 57.29578

class MahonyAHRS:
    """
    Madgwick's implementation of Mahony's AHRS algorithm.
    
    See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
    
    Original C implementation by Sebastian Madgwick
    Python implementation by [Your name]
    """
    
    def __init__(self, sample_freq=200.0, kp=0.5, ki=0.0):
        """
        Initialize the AHRS algorithm.
        
        Args:
            sample_freq: Sample frequency in Hz
            kp: Proportional gain
            ki: Integral gain
        """
        self.sample_freq = sample_freq
        self.two_kp = 2.0 * kp
        self.two_ki = 2.0 * ki
        
        # quaternion of sensor frame relative to auxiliary frame
        self.q0 = 1.0
        self.q1 = 0.0
        self.q2 = 0.0
        self.q3 = 0.0
        
        # integral error terms scaled by Ki
        self.integral_fb_x = 0.0
        self.integral_fb_y = 0.0
        self.integral_fb_z = 0.0
    
    def update(self, gx, gy, gz, ax, ay, az, mx, my, mz):
        """
        AHRS algorithm update with magnetometer.
        
        Args:
            gx, gy, gz: Gyroscope measurements in rad/s
            ax, ay, az: Accelerometer measurements in any consistent unit
            mx, my, mz: Magnetometer measurements in any consistent unit
        """
        # Use IMU algorithm if magnetometer measurement invalid
        if (mx == 0.0) and (my == 0.0) and (mz == 0.0):
            self.update_imu(gx, gy, gz, ax, ay, az)
            return
        
        # Check if accelerometer measurement is valid
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
            hx = 2.0 * (mx * (0.5 - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2))
            hy = 2.0 * (mx * (q1q2 + q0q3) + my * (0.5 - q1q1 - q3q3) + mz * (q2q3 - q0q1))
            bx = math.sqrt(hx * hx + hy * hy)
            bz = 2.0 * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5 - q1q1 - q2q2))
            
            # Estimated direction of gravity and magnetic field
            half_vx = q1q3 - q0q2
            half_vy = q0q1 + q2q3
            half_vz = q0q0 - 0.5 + q3q3
            half_wx = bx * (0.5 - q2q2 - q3q3) + bz * (q1q3 - q0q2)
            half_wy = bx * (q1q2 - q0q3) + bz * (q0q1 + q2q3)
            half_wz = bx * (q0q2 + q1q3) + bz * (0.5 - q1q1 - q2q2)
            
            # Error is sum of cross product between estimated direction and measured direction of field vectors
            half_ex = (ay * half_vz - az * half_vy) + (my * half_wz - mz * half_wy)
            half_ey = (az * half_vx - ax * half_vz) + (mz * half_wx - mx * half_wz)
            half_ez = (ax * half_vy - ay * half_vx) + (mx * half_wy - my * half_wx)
            
            # Compute and apply integral feedback if enabled
            if self.two_ki > 0.0:
                self.integral_fb_x += self.two_ki * half_ex * (1.0 / self.sample_freq)
                self.integral_fb_y += self.two_ki * half_ey * (1.0 / self.sample_freq)
                self.integral_fb_z += self.two_ki * half_ez * (1.0 / self.sample_freq)
                gx += self.integral_fb_x
                gy += self.integral_fb_y
                gz += self.integral_fb_z
            else:
                self.integral_fb_x = 0.0
                self.integral_fb_y = 0.0
                self.integral_fb_z = 0.0
            
            # Apply proportional feedback
            gx += self.two_kp * half_ex
            gy += self.two_kp * half_ey
            gz += self.two_kp * half_ez
        
        # Integrate rate of change of quaternion
        gx *= (0.5 * (1.0 / self.sample_freq))
        gy *= (0.5 * (1.0 / self.sample_freq))
        gz *= (0.5 * (1.0 / self.sample_freq))
        qa = self.q0
        qb = self.q1
        qc = self.q2
        self.q0 += (-qb * gx - qc * gy - self.q3 * gz)
        self.q1 += (qa * gx + qc * gz - self.q3 * gy)
        self.q2 += (qa * gy - qb * gz + self.q3 * gx)
        self.q3 += (qa * gz + qb * gy - qc * gx)
        
        # Normalize quaternion
        recip_norm = self.inv_sqrt(self.q0 * self.q0 + self.q1 * self.q1 + self.q2 * self.q2 + self.q3 * self.q3)
        self.q0 *= recip_norm
        self.q1 *= recip_norm
        self.q2 *= recip_norm
        self.q3 *= recip_norm
    
    def update_imu(self, gx, gy, gz, ax, ay, az):
        """
        IMU algorithm update (no magnetometer).
        
        Args:
            gx, gy, gz: Gyroscope measurements in rad/s
            ax, ay, az: Accelerometer measurements in any consistent unit
        """
        # Check if accelerometer measurement is valid
        if not ((ax == 0.0) and (ay == 0.0) and (az == 0.0)):
            # Normalize accelerometer measurement
            recip_norm = self.inv_sqrt(ax * ax + ay * ay + az * az)
            ax *= recip_norm
            ay *= recip_norm
            az *= recip_norm
            
            # Estimated direction of gravity and vector perpendicular to magnetic flux
            half_vx = self.q1 * self.q3 - self.q0 * self.q2
            half_vy = self.q0 * self.q1 + self.q2 * self.q3
            half_vz = self.q0 * self.q0 - 0.5 + self.q3 * self.q3
            
            # Error is sum of cross product between estimated and measured direction of gravity
            half_ex = (ay * half_vz - az * half_vy)
            half_ey = (az * half_vx - ax * half_vz)
            half_ez = (ax * half_vy - ay * half_vx)
            
            # Compute and apply integral feedback if enabled
            if self.two_ki > 0.0:
                self.integral_fb_x += self.two_ki * half_ex * (1.0 / self.sample_freq)
                self.integral_fb_y += self.two_ki * half_ey * (1.0 / self.sample_freq)
                self.integral_fb_z += self.two_ki * half_ez * (1.0 / self.sample_freq)
                gx += self.integral_fb_x
                gy += self.integral_fb_y
                gz += self.integral_fb_z
            else:
                self.integral_fb_x = 0.0
                self.integral_fb_y = 0.0
                self.integral_fb_z = 0.0
            
            # Apply proportional feedback
            gx += self.two_kp * half_ex
            gy += self.two_kp * half_ey
            gz += self.two_kp * half_ez
        
        # Integrate rate of change of quaternion
        gx *= (0.5 * (1.0 / self.sample_freq))
        gy *= (0.5 * (1.0 / self.sample_freq))
        gz *= (0.5 * (1.0 / self.sample_freq))
        qa = self.q0
        qb = self.q1
        qc = self.q2
        self.q0 += (-qb * gx - qc * gy - self.q3 * gz)
        self.q1 += (qa * gx + qc * gz - self.q3 * gy)
        self.q2 += (qa * gy - qb * gz + self.q3 * gx)
        self.q3 += (qa * gz + qb * gy - qc * gx)
        
        # Normalize quaternion
        recip_norm = self.inv_sqrt(self.q0 * self.q0 + self.q1 * self.q1 + self.q2 * self.q2 + self.q3 * self.q3)
        self.q0 *= recip_norm
        self.q1 *= recip_norm
        self.q2 *= recip_norm
        self.q3 *= recip_norm
    
    def inv_sqrt(self, x):
        """
        Fast inverse square-root.
        See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
        
        Args:
            x: Input value
            
        Returns:
            Inverse square root of x
        """
        # Note: In Python, we'll use the numpy version for accuracy and portability
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
        
        # yaw = -math.atan2(2 * (self.q1 * self.q2 + self.q0 * self.q3), self.q0 * self.q0 + self.q1 * self.q1 - self.q2 * self.q2 - self.q3 * self.q3) * RAD2DEG
        # pitch = math.asin(2 * (self.q1 * self.q3 - self.q0 * self.q2)) * RAD2DEG
        # roll = math.atan2(2 * (self.q0 * self.q1 + self.q2 * self.q3), self.q0 * self.q0 - self.q1 * self.q1 - self.q2 * self.q2 + self.q3 * self.q3) * RAD2DEG

        return (roll* RAD2DEG, pitch* RAD2DEG, yaw* RAD2DEG)


# Example usage
if __name__ == "__main__":
    # Create AHRS object with default settings
    ahrs = MahonyAHRS(sample_freq=100.0, kp=0.5, ki=0.0)
    
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