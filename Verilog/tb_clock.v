`timescale 1ns / 1ps

module tb_digital_clock_with_menu;
  // Inputs
  reg clk, reset;
  reg mode_24h, set_time, set_alarm, start_timer, pause_timer;
  reg [5:0] in_hour, in_minute, in_second;
  reg [5:0] in_alarm_hr, in_alarm_min;
  reg [3:0] in_timer_min;
  reg sw_start, sw_stop, sw_reset;
  reg [2:0] menu_select;
  reg [4:0] in_day;
  reg [3:0] in_month;
  reg [11:0] in_year;
  // Outputs
  wire [5:0] hour, minute, second;
  wire am_pm;
  wire [4:0] day;
  wire [3:0] month;
  wire [11:0] year;
  wire timer_buzzer, alarm_buzzer;
  wire [5:0] sw_min, sw_sec;
  wire [6:0] sw_hun;
  // Instantiate DUT with fast-sim dividers
  digital_clock_with_menu #(
    .CLK_PER_SEC(10),  // “1 Hz” every 10 clocks
    .CLK100_PER_SEC(2)   // “100 Hz” every 2 clocks
  ) uut (
    .clk(clk),
    .reset(reset),
    .mode_24h(mode_24h),
    .set_time(set_time),
    .in_hour(in_hour),
    .in_minute(in_minute),
    .in_second(in_second),
    .set_alarm(set_alarm),
    .in_alarm_hr(in_alarm_hr),
    .in_alarm_min(in_alarm_min),
    .start_timer(start_timer),
    .pause_timer(pause_timer),
    .in_timer_min(in_timer_min),
    .sw_start(sw_start),
    .sw_stop(sw_stop),
    .sw_reset(sw_reset),
    .menu_select(menu_select),
    .in_day(in_day),
    .in_month(in_month),
    .in_year(in_year),
    .hour(hour),
    .minute(minute),
    .second(second),
    .am_pm(am_pm),
    .day(day),
    .month(month),
    .year(year),
    .timer_buzzer(timer_buzzer),
    .alarm_buzzer(alarm_buzzer),
    .sw_min(sw_min),
    .sw_sec(sw_sec),
    .sw_hun(sw_hun)
  );
  // 100 MHz clock → 10 ns period
  initial clk = 0;
  always #5 clk = ~clk;
  initial begin
    $dumpfile("gtk.vcd");
    $dumpvars(0, tb_digital_clock_with_menu);
    // Resetting initially
    reset = 1;
    menu_select = 0;
    mode_24h = 1;
    set_time = 0;
    set_alarm = 0;
    start_timer = 0;
    pause_timer = 0;
    sw_start = 0;
    sw_stop = 0;
    sw_reset = 0;
    #20 reset = 0;
    // MODE 0: 24 h rollover
    $display("\n24-hour clock");
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd50;
    set_time = 1; #10 set_time = 0;
    repeat (14) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
    // MODE 0: 12 h AM/PM toggle
    $display("\n12-hour clock");
    reset = 1; #10 reset = 0;
    mode_24h = 0;
    in_hour = 6'd11;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    repeat (10) begin
      #100;
      $display("  %02d:%02d:%02d %s", hour, minute, second, am_pm ? "PM" : "AM");
    end
    // MODE 1: Timer (1 min) with pause/unpause
    $display("\n1-minute timer");
    reset = 1; #10 reset = 0;
    menu_select = 3'd1;
    in_timer_min = 4'd1;
    start_timer = 1; #10 start_timer  = 0;
    // first 3 ticks:
    $display("Starting countdown ... ");
    repeat (3) begin
      #100;
      $display("timer=%2d buzzer_state=%b", uut.timer_count, timer_buzzer);
    end
    // pause for one tick
    pause_timer = 1; #10 pause_timer = 0;
    #100;
    $display("Paused ... timer=%2d  buzzer_state=%b", uut.timer_count, timer_buzzer);
    // unpause and finish three more ticks
    pause_timer = 1; #10 pause_timer = 0;
    repeat (59) begin
      #100;
      $display("timer=%2d  buzzer_state=%b", uut.timer_count, timer_buzzer);
    end
    // MODE 2: Alarm (sample through 07:00:00)
    $display("\nAlarm set at 07:00:00");
    reset = 1; #10 reset = 0;
    menu_select = 3'd0;
    in_hour = 6'd6;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    menu_select = 3'd2;
    in_alarm_hr = 6'd7;
    in_alarm_min = 6'd0;
    set_alarm = 1; #10 set_alarm = 0;
    repeat (20) begin
      #100;
      $display("  %02d:%02d:%02d  alarm_buzzer=%b", hour, minute, second, alarm_buzzer);
    end
    // MODE 3: Date set & display
    $display("\nSetting date to 28-2-2024");
    $display("Before setting - \n Date = %0d:%0d:%0d", day, month, year);
    reset = 1; #10 reset = 0;
    menu_select = 3'd3;
    in_day = 5'd30;
    in_month = 4'd6;
    in_year = 12'd2024;
    set_time = 1; #10 set_time = 0;
    #100;
    $display("After setting - \nDate = %0d:%0d:%0d", day, month, year);
    // MODE 0: 24 h rollover
    $display("\nSetting time after setting date");
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd50;	
    set_time = 1; #10 set_time = 0;
    repeat (14) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
    $display("Resetting date to 1-1-20 after 30-4-25");
    reset = 1; #10 reset = 0;
    menu_select = 3'd3;
    in_day = 5'd30;
    in_month = 4'd4;
    in_year = 12'd2025;
    set_time = 1; #10 set_time = 0;
    #100;
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    repeat (10) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
     $display("\n28-2 to 1-3 during non-leap years");
     reset = 1; #10 reset = 0;
    menu_select  = 3'd3;
    in_day = 5'd28;
    in_month = 4'd2;
    in_year = 12'd2023;
    set_time = 1; #10 set_time = 0;
    #100;
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    repeat (10) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
    $display("\n28-2 to 29-2 during leap years");
     reset = 1; #10 reset = 0;
    menu_select = 3'd3;
    in_day = 5'd28;
    in_month = 4'd2;
    in_year = 12'd2020;
    set_time = 1; #10 set_time = 0;
    #100;
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    repeat (10) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
    $display("\n29-2 to 1-3 during leap years");
     reset = 1; #10 reset = 0;
    menu_select = 3'd3;
    in_day = 5'd29;
    in_month = 4'd2;
    in_year = 12'd2020;
    set_time = 1; #10 set_time = 0;
    #100;
    menu_select = 3'd0;
    mode_24h = 1;
    in_hour = 6'd23;
    in_minute = 6'd59;
    in_second = 6'd55;
    set_time = 1; #10 set_time = 0;
    repeat (10) begin
      #100;
      $display("  %02d:%02d:%02d  %02d-%02d-%04d", hour, minute, second, day, month, year);
    end
    // MODE 4: Stopwatch 2.00 s
    $display("\nStopwatch for 2s");
    reset = 1; #10 reset = 0;
    menu_select = 3'd4;
    sw_reset = 1; #10 sw_reset = 0;
    sw_start = 1;
    repeat (200) #20;  // 200 ticks @20 ns = 2.00 µs sim → 2.00 s logical
    $display("%02d:%02d:%02d", sw_min, sw_sec, sw_hun);
    sw_start = 0;
    #100;
    #20 $finish;
  end

endmodule
