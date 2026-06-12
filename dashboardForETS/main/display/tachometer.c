#include "lvgl.h"
#include "include/tachometer.h"

lv_obj_t *scale;
lv_obj_t *needle;

void create_tachometer(void) {
    // 1. Create the Scale object
    scale = lv_scale_create(lv_screen_active());
    lv_obj_set_size(scale, 120, 120);
    lv_obj_center(scale);

    // Make the scale round (inner)
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);

    // Rotation: 0 degrees = 3 o'clock. Start from lower-left (150 deg)
    // and span 240 degrees to lower-right
    lv_scale_set_rotation(scale, 150);
    lv_scale_set_angle_range(scale, 240);

    // Range 0-80 (RPM x 100)
    lv_scale_set_range(scale, 0, 80);

    // Show labels on major ticks
    lv_scale_set_label_show(scale, true);

    // Tick configuration:
    // total_tick_count = total number of minor tick intervals (including major tick positions)
    // major_tick_every = every Nth tick is a major tick
    // For 0..80 range with labels every 10: 9 major ticks, with 5 minor ticks between each = 8*5+8 = 48 intervals
    // Actually with 9 major ticks and 5 minor increments between each, total = (9-1)*5 + 1 = 41 ticks total,
    // but simpler: just set 41 ticks total, major every 5th
    lv_scale_set_total_tick_count(scale, 41);
    lv_scale_set_major_tick_every(scale, 5);

    // RED ZONE (Sections)
    lv_scale_section_t * red_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, red_section, 60, 80);

    // Style the red section (indicator = major ticks & labels, items = minor ticks)
    static lv_style_t section_label_style;
    lv_style_init(&section_label_style);
    lv_style_set_text_color(&section_label_style, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_line_color(&section_label_style, lv_palette_main(LV_PALETTE_RED));
    lv_scale_set_section_style_indicator(scale, red_section, &section_label_style);

    // 2. CREATE THE NEEDLE (Line inside the scale)
    needle = lv_line_create(scale);

    // Needle coordinates: from center [60,60] upward to [60,15]
    static lv_point_precise_t needle_points[] = { {60, 60}, {60, 15} };
    lv_line_set_points(needle, needle_points, 2);

    // Needle style
    lv_obj_set_style_line_width(needle, 4, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(needle, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(needle, lv_color_black(), LV_PART_MAIN);

    // The needle pivot is at the center of the scale (60, 60)
    lv_obj_set_style_transform_pivot_x(needle, 60, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(needle, 60, LV_PART_MAIN);
}