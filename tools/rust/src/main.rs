extern crate piston_window;
extern crate plotters;
extern crate serialport;

use std::io::prelude::*;
use piston_window::*;
use plotters::prelude::*;
use serialport::*;

fn main() {
    let mut window: PistonWindow = WindowSettings::new("IMU Waveform", [640, 480])
        .exit_on_esc(true)
        .build()
        .unwrap();

    let mut port = serialport::new("/dev/cu.usbmodem578E0044521", 115200)
        .timeout(std::time::Duration::from_millis(10))
        .open()
        .expect("Failed to open serial port");

    let mut serial_buf: Vec<u8> = vec![0; 1024];
    let mut data_buffer: Vec<f32> = Vec::new();

    while let Some(event) = window.next() {
        let num_bytes = port.read(&mut serial_buf)
            .expect("Failed to read from serial port");

        if num_bytes > 0 {
            let data_str = String::from_utf8_lossy(&serial_buf[..num_bytes]);
            if let Some(imu_data) = parse_imu_data(&data_str) {
                data_buffer.extend(imu_data);
                window.draw_2d(&event, |context, graphics, _device| {
                    clear([1.0; 4], graphics);
                    plot_waveform(&data_buffer, &context, graphics);
                });
            }
        }
    }
}

fn parse_imu_data(data_str: &str) -> Option<Vec<f32>> {
    let mut data_vec: Vec<f32> = Vec::new();
    for value in data_str.strip_prefix("BETA FLIGHT IMU imu:").unwrap_or("").split(", ") {
        if let Ok(value_float) = value.parse::<f32>() {
            data_vec.push(value_float);
        }
    }
    if data_vec.len() == 8 {
        Some(data_vec)
    } else {
        None
    }
}


fn plot_waveform<G: Graphics>(data: &[f32], context: &Context, graphics: &mut G) {
    let root = BitMapBackend::new("waveform.png", (640, 480));
    let root_area = root.into_drawing_area();
    let mut chart = ChartBuilder::on(&root_area)
        .caption("IMU Data Waveform", ("sans-serif", 50).into_font())
        .set_label_area_size(LabelAreaPosition::Left, 40)
        .set_label_area_size(LabelAreaPosition::Bottom, 40)
        .build_cartesian_2d(0f64..data.len() as f64, -10.0..10.0)
        .unwrap();

    chart
        .configure_mesh()
        .x_desc("Samples")
        .y_desc("Value")
        .axis_desc_style(("sans-serif", 15))
        .draw()
        .unwrap();

    chart.draw_series(
        data.iter().enumerate().map(|(i, y)| Circle::new((i as f64, *y as f64), 1, &RED)),
    ).unwrap();

    root_area.present_into(&context, &graphics).unwrap();
}



