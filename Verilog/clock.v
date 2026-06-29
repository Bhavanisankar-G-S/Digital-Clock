module digital_clock_with_menu #(
    parameter CLK_PER_SEC = 50_000_000,
    parameter CLK100_PER_SEC = 500_000
) (
    input clk,
    input reset,
    input mode_24h,
    // Time set inputs
    input set_time,
    input [5:0] in_hour,
    input [5:0] in_minute,
    input [5:0] in_second,
    // Alarm set inputs
    input set_alarm,
    input [5:0] in_alarm_hr,
    input [5:0] in_alarm_min,
    // Timer inputs
    input start_timer,
    input pause_timer,
    input [3:0] in_timer_min,
    // Stopwatch inputs
    input sw_start,
    input sw_stop,
    input sw_reset,
    input [2:0] menu_select,
    // Mode 3 date set
    input [4:0] in_day,
    input [3:0] in_month,
    input [11:0] in_year,
    // Outputs
    output reg [5:0] hour,
    output reg [5:0] minute,
    output reg [5:0] second,
    output reg am_pm,
    output reg [4:0] day,
    output reg [3:0] month,
    output reg [11:0] year,
    output reg timer_buzzer,
    output reg alarm_buzzer,
    output reg [5:0] sw_min,
    output reg [5:0] sw_sec,
    output reg [6:0] sw_hun
);
    reg [5:0] alarm_hr_r, alarm_min_r;
    reg [11:0] timer_count;
    reg timer_running, timer_paused, stopwatch_running, tick_1hz, tick_100hz;
    reg [25:0] clk_div;
    reg [18:0] clk_div_100hz;
    reg [3:0] alarm_ring_timer = 0; 
    // 1 Hz & 100 Hz dividers
    // Asynchronous reset
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            clk_div <= 0;
	    tick_1hz <= 0;
            clk_div_100hz <= 0; 
	    tick_100hz <= 0;
        end else begin
            clk_div <= (clk_div == CLK_PER_SEC-1) ? 0 : clk_div + 1;
            tick_1hz <= (clk_div == CLK_PER_SEC-1);
            clk_div_100hz <= (clk_div_100hz == CLK100_PER_SEC-1) ? 0 : clk_div_100hz + 1;
            tick_100hz <= (clk_div_100hz == CLK100_PER_SEC-1);
        end
    end
    // Leap-year condition
    function is_leap_year(input [11:0] y);
        is_leap_year = ((y%4==0)&&(y%100!=0))||(y%400==0) ;
    endfunction
    // Days-in-month
    function [5:0] days_in_month(input [3:0] m, input [11:0] y);
        case(m)
            4'd1,4'd3,4'd5,4'd7,4'd8,4'd10,4'd12: days_in_month = 31;
            4'd4,4'd6,4'd9,4'd11: days_in_month = 30;
            4'd2: days_in_month = is_leap_year(y) ? 29 : 28;
            default: days_in_month = 30;
        endcase
    endfunction
    // Main logic
    reg next_is_midnight;
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            hour <= 12; 
	    minute <= 0; 
	    second <= 0; 
	    am_pm <= 0;
            day <= 1; 
	    month <= 1; 
	    year <= 2020;
            alarm_hr_r <= 0; 
	    alarm_min_r <= 0; 
	    alarm_buzzer <= 0;
            timer_count <= 0; 
	    timer_running <= 0; 
	    timer_paused <= 0; 
	    timer_buzzer <= 0;
            sw_min <= 0; 
	    sw_sec <= 0; 
	    sw_hun <= 0; 
	    stopwatch_running <= 0;
        end else begin
            next_is_midnight = 0;  // Initialize next_is_midnight to 0
            // Advance time on tick_1hz
            if (set_time && menu_select == 3'd0) begin
                hour   <= in_hour;
                minute <= in_minute;
                second <= in_second;
                am_pm  <= (!mode_24h && in_hour >= 12);
            end else if (tick_1hz) begin
                if (mode_24h) begin
                    next_is_midnight = (hour == 6'd23 && minute == 6'd59 && second == 6'd59);
                end else begin
                    next_is_midnight = (hour == 6'd11 && minute == 6'd59 && second == 6'd59 && am_pm == 1);
                end
                // Incrementing time
                if (second == 6'd59) begin
                    second <= 0;
                    if (minute == 6'd59) begin
                        minute <= 0;
			// AM/PM 
                        if (mode_24h) begin
                            if (hour == 6'd23) hour <= 0;
                            else hour <= hour + 1;
                        end else begin
                            if (hour == 6'd11) begin hour <= 12; am_pm <= ~am_pm; end
                            else if (hour == 6'd12) hour <= 1;
                            else hour <= hour + 1;
                        end
                    end else minute <= minute + 1;
                end else second <= second + 1;
                // Increasing date if midnight is crossed
                if (next_is_midnight) begin
                    if (day == days_in_month(month,year)) begin
                        day <= 1;
                        if (month == 12) begin month <= 1; year <= year + 1; end
                        else month <= month + 1;
                    end else day <= day + 1;
		// Resetting date as per question
                if ( year > 2025 || (year == 2025 && (month > 4 || (month == 4 && day >= 30))) ) begin
                        day <= 1; month <= 1; year <= 2020;
                    end
                end
            end
            case(menu_select)
                3'd1: begin // Timer
                    if (start_timer) begin
                        timer_count <= in_timer_min * 60;
                        timer_running <= 1;
                        timer_paused <= 0;
                        timer_buzzer <= 0;
                    end else if (pause_timer) timer_paused <= ~timer_paused;
                    if (tick_1hz && timer_running && !timer_paused) begin
                        if (timer_count > 0) timer_count <= timer_count - 1;
                        else begin timer_running <= 0; timer_buzzer <= 1; end
                    end
                end
                3'd2: begin // Alarm
    			if (set_alarm) begin
        			alarm_hr_r <= in_alarm_hr;
        			alarm_min_r <= in_alarm_min;
    			end
		end
                3'd3: begin // Date set
                    if (set_time) begin
                        day   <= in_day;
                        month <= in_month;
                        year  <= in_year;
                    end
                end
                3'd4: begin // Stopwatch
                    if (sw_reset) begin
                        sw_min <= 0;
			sw_sec <= 0; 
			sw_hun <= 0; 
			stopwatch_running <= 0;
                    end else if (sw_start) stopwatch_running <= 1;
                    else if (sw_stop) stopwatch_running <= 0;
                    if (tick_100hz && stopwatch_running) begin
                        if (sw_hun == 99) begin
                            sw_hun <= 0;
                            if (sw_sec == 59) begin
                                sw_sec <= 0;
                                if (sw_min == 59) sw_min <= 0;
                                else sw_min <= sw_min + 1;
                            end else sw_sec <= sw_sec + 1;
                        end else sw_hun <= sw_hun + 1;
                    end
                end
             endcase
// Ringing alarm for 10 seconds
if (tick_1hz) begin
    if (hour == alarm_hr_r && minute == alarm_min_r && second == 0)
        alarm_ring_timer <= 4'd10;
    else if (alarm_ring_timer > 0)
        alarm_ring_timer <= alarm_ring_timer - 1;

    alarm_buzzer <= (alarm_ring_timer > 0);
end
end
end
endmodule
