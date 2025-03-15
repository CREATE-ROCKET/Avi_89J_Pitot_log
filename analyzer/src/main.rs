//#!/usr/bin/env -S cargo +nightly -q -Zscript
/*
---
[package]
name = "rust"
version = "0.1.0"
edition = "2024"

[dependencies]
clap = { version = "4.5.31", features = ["derive"] }
csv = "1.3.1"
plotters = "0.3.7"
---
*/

// --------------------------------------------------------------------
// pitot data analyzer written in rust
// --------------------------------------------------------------------

extern crate clap;
extern crate csv;
extern crate plotters;
use clap::Parser;
use plotters::{
    chart::ChartBuilder,
    prelude::IntoDrawingArea,
    style::{IntoFont, RED, WHITE},
};
use std::fs::File;

/// ピトー管のデータを解析するプログラム
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// データが入っているファイルのパス
    #[arg(short, long, default_value_t = String::from("./data.csv"))]
    file_path: String,

    /// 解析したデータを入れるファイル名
    #[arg(short, long, default_value_t = String::from("./out/figure1.png"))]
    output: String,

    /// 打ち上げ時の圧力
    #[arg(short, long, default_value_t = 101.33E3)]
    static_pascal: f32,
}

fn graph_print(
    x: Vec<i64>,
    y: Vec<f32>,
    graph_name: &str,
    output_name: &str,
) -> Result<(), Box<dyn std::error::Error>> {
    let width = 1080; // 横
    let height = 720; // 縦
    let graph_path_name = format!("{}_{}.png", output_name, graph_name);
    let root = plotters::prelude::BitMapBackend::new(&graph_path_name, (width, height))
        .into_drawing_area();
    root.fill(&WHITE)?;

    // y軸の最大最小値を算出
    let (y_min, y_max) = y
        .iter()
        .fold((0.0 / 0.0, 0.0), |(m, n), v| (v.min(m), v.max(n)));

    let caption = graph_name;
    let font = ("sans-serif", 20);

    let mut chart = ChartBuilder::on(&root)
        .caption(caption, font.into_font())
        .margin(20)
        .x_label_area_size(16)
        .y_label_area_size(42)
        .build_cartesian_2d(*x.first().unwrap()..*x.last().unwrap(), y_min..y_max)?;

    chart.configure_mesh().draw()?;
    let line_series =
        plotters::series::LineSeries::new(x.iter().zip(y.iter()).map(|(x, y)| (*x, *y)), &RED);
    chart.draw_series(line_series)?;

    Ok(())
}

fn main() {
    // ファイル名を受け取る
    let args = Args::parse();

    // file_path をパスとして csv の読み取り
    let pitot_data = File::open(args.file_path).expect("invalid file path");
    let mut pitot_data = csv::Reader::from_reader(pitot_data);
    let mut time_data: Vec<i64> = Vec::new();
    let mut pascal_data: Vec<f32> = Vec::new();
    let mut temperature_data: Vec<f32> = Vec::new();
    for result in pitot_data.records() {
        let record = result.unwrap();
        // 読み取ったcsvのStringRecordからtemperatureとpascal、 timeを読み取る
        time_data.push(
            record[0]
                .trim()
                .parse::<i64>()
                .expect("failed to parse time data"),
        );
        pascal_data.push(
            record[1]
                .trim()
                .parse::<f32>()
                .expect("failed to parse pascal data"),
        );
        temperature_data.push(
            record[2]
                .trim()
                .parse::<f32>()
                .expect("failed to parse temperature data"),
        );
    }

    assert_eq!(time_data.len(), pascal_data.len());
    assert_eq!(time_data.len(), temperature_data.len());

    let mut speed_data: Vec<f32> = Vec::new();
    for (pascal, temp) in pascal_data
        .clone()
        .into_iter()
        .zip(temperature_data.clone().into_iter())
    {
        // 密度計算
        let rho = 0.0034837 * args.static_pascal / (temp + 273.15);
        let result = (2.0 * pascal / rho).sqrt();
        speed_data.push(result);
    }

    assert_eq!(time_data.len(), speed_data.len());

    graph_print(
        time_data.clone(),
        temperature_data.clone(),
        "temp_data",
        &args.output,
    )
    .unwrap();
    graph_print(
        time_data.clone(),
        pascal_data.clone(),
        "pascal_data",
        &args.output,
    )
    .unwrap();
    graph_print(
        time_data.clone(),
        speed_data.clone(),
        "speed_data",
        &args.output,
    )
    .unwrap();

    let mut speed_data_limited: Vec<f32> = Vec::new();
    let mut time_data_limited: Vec<i64> = Vec::new();

    for (speed, time) in speed_data
        .clone()
        .into_iter()
        .zip(time_data.clone().into_iter())
    {
        if speed > 10.0 {
            speed_data_limited.push(speed);
            time_data_limited.push(time);
        }
    }

    graph_print(
        time_data_limited.clone(),
        speed_data_limited.clone(),
        "speed_data_lim",
        &args.output,
    )
    .unwrap();
}
