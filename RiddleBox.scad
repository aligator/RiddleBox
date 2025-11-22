// Wooden box with false bottom, secret drawer and RCA cutouts in lid

// ---------- Feature toggles ----------
show_box_body     = true;
show_lid          = true;
show_false_bottom = true;
show_drawer       = true;
show_drawer_cut   = true;
show_rca_cutouts  = true;
show_lock         = true;

show_only_false_bottom_lasercut = false;

// ---------- Parameters ----------
// Master dimensions
// outer length (X, mm)
outer_len = 300;
// outer width (Y, mm)
outer_w   = 200;
// outer height (Z, mm)
outer_h   = 140;

// inner height (Z, mm)
inner_h   = 85;

// Wall thickness
// left/right walls (X)
wall_x = 12.5;
// front/back walls (Y)
wall_y = 12.5;

// Base and lid
// lid block thickness
lid_th  = 32;  // bottom thickness
base_th = outer_h-inner_h-lid_th;

// size of the false-bottom supports
false_bottom_support_width = 10;


// Hidden compartment
// hidden drawer height (Z)
hidden_h        = 20;
// false bottom thickness
false_bottom_th = 5;
// drawer length in X direction (< inner_len)
drawer_len      = 120;
// space between the drawer and the front wall. (For example to make space for the lock electronic if the lock goes into the false-bottom)
drawer_y_space  = 14;
// clearance for drawer (mm)
drawer_clear    = 1.0;
// wall thickness of drawer
drawer_wall     = 3;
// hole for the power cable
drawer_power_hole_d = 10;
// hole offset to the left (so that the cable does not collide with the internal electronic
drawer_power_hole_offset_to_left = 0;
drawer_power_hole_offset_to_top = 0;

// Cable
// Cable coutouts in the false bottom and the drawer
cable_cutout = 15;

// Derived dimensions
inner_len = outer_len - 2*wall_x;
inner_w   = outer_w   - 2*wall_y;
visible_inner_h   = outer_h - base_th - lid_th - hidden_h - false_bottom_th;

// hollow inside lid
lid_cutout = 24;

// RCA cutouts (lid)
// hole diameter for RCA jacks
rca_hole_d = 8;
// number of columns per block
rca_cols   = 2;
// number of rows per block
rca_rows   = 5;
// spacing between columns
rca_pitch_x = 20;
// spacing between rows
rca_pitch_y = 20;
// margin from left/right side
rca_margin_side  = 80;

// Display
// degrees, 0 = closed, 100 = open
lid_open_angle = 100; 

// Electronic Lock height (only the base shape for vizualization)
lock_ht=73;
lock_width=13.1;
lock_len=58;
lock_shift_up=0;

lock_false_bottom_cutout_margin=1;

c0=0.01+0;

// ---------- Dimension printout ----------
echo("Inner length (X) =", inner_len, "mm");
echo("Inner width (Y)  =", inner_w, "mm");
echo("Visible inner height (Z) =", visible_inner_h, "mm");
echo("Hidden drawer height (Z) =", hidden_h, "mm");
echo("False bottom thickness (Z) =", false_bottom_th, "mm");
echo("Total internal height (Z) =", visible_inner_h + hidden_h + false_bottom_th, "mm");

echo("lock space left", visible_inner_h - lock_ht+lock_shift_up);

// ---------- Modules ----------
module box_body(){
    difference(){
        // Outer shell without lid
        cube([outer_len, outer_w, outer_h - lid_th], center=false);

        // Hollow interior from base up to lid
        translate([wall_x, wall_y, base_th])
            cube([inner_len, inner_w, outer_h - lid_th - base_th + c0], center=false);

        // Rear cutout for drawer access
        if (show_drawer_cut)
            translate([wall_x + inner_len/2 - drawer_len/2, -c0, base_th])
                cube([drawer_len, wall_y+2+c0, hidden_h], center=false);
    }
    
    translate([wall_x, wall_y, base_th])
        cube([false_bottom_support_width, inner_w, hidden_h]);
    translate([inner_len, wall_y, base_th])
        cube([false_bottom_support_width, inner_w, hidden_h]);

}

module lock(margin=0) {
    translate([outer_len/2-lock_len/2-margin,
               wall_y+inner_w-lock_width-margin,
               base_th + hidden_h + false_bottom_th+visible_inner_h - lock_ht + lock_shift_up])
        cube([lock_len+margin*2, lock_width+margin*2, lock_ht], center=false);
}

module lid_block(){
    difference(){
        cube([outer_len, outer_w, lid_th], center=false);

        // Hollow inside lid
        translate([wall_x, wall_y, -c0])
            cube([inner_len, inner_w, lid_cutout+c0], center=false);

        // RCA cutouts in lid
        if (show_rca_cutouts)
            translate([0, 0, -lid_th])
            rca_cutouts();
    }
}

module drawer(){
    difference(){
        // Outer size = hidden space minus clearance
        cube([drawer_len-2*drawer_clear,
              inner_w + wall_y - drawer_y_space,
              hidden_h-drawer_clear], center=false);

        // Inner hollow, top open
        translate([drawer_wall, drawer_wall, drawer_wall])
            cube([drawer_len-2*drawer_clear-2*drawer_wall,
                  inner_w+wall_y - drawer_wall*2 - drawer_y_space,
                  hidden_h-drawer_clear], center=false);

        // Power cable hole in front
        translate([(drawer_len-2*drawer_clear)/2-drawer_power_hole_offset_to_left,
                   drawer_wall+c0,
                   (hidden_h-drawer_clear)/2+drawer_power_hole_offset_to_top])
            rotate([90, 0, 0])
            cylinder(d=drawer_power_hole_d, h=drawer_wall+2*c0);
        
        // Cable
        translate([-c0, drawer_wall-cable_cutout+inner_w+wall_y - drawer_wall*2 - drawer_y_space, drawer_wall])
        cube([drawer_wall+2*c0, cable_cutout, hidden_h+c0]);
    }
}

module false_bottom(){
    // Plate above the hidden drawer with clearance
    difference() {
        translate([wall_x+0.5, wall_y+0.5, base_th + hidden_h]) {
            difference() {
                cube([inner_len-1, inner_w-1, false_bottom_th], center=false);
                translate([false_bottom_support_width, 0, -c0])
                    cube([cable_cutout, cable_cutout, false_bottom_th+2*c0]);
            }
        }
        
        lock(lock_false_bottom_cutout_margin);
    }
}


module false_bottom(){
    // Plate above the hidden drawer with clearance
    difference() {
        translate([wall_x+0.5, wall_y+0.5, base_th + hidden_h]) {
            difference() {
                cube([inner_len-1, inner_w-1, false_bottom_th], center=false);
                translate([false_bottom_support_width, 0, -c0])
                    cube([cable_cutout, cable_cutout, false_bottom_th+2*c0]);
            }
        }
        
        lock(lock_false_bottom_cutout_margin);
    }
}

module false_bottom_lasercut() {
    projection(cut = false) false_bottom();
}

// RCA cutout blocks (centered along Y axis of lid)
module rca_block(x_offset){
    total_height = (rca_rows-1)*rca_pitch_y;
    y_start = outer_w/2 - total_height/2;
    for (row=[0:rca_rows-1]){
        for (col=[0:rca_cols-1]){
            translate([x_offset + col*rca_pitch_x,
                       y_start + row*rca_pitch_y,
                       lid_th - c0])
                cylinder(d=rca_hole_d, h=lid_th+2*c0, center=false);
        }
    }
}

module rca_cutouts(){
    // Left block
    rca_block(rca_margin_side);
    // Right block
    rca_block(outer_len - rca_margin_side - (rca_cols-1)*rca_pitch_x);
}

// ---------- Assembly ----------
module assembly(){
    if (show_box_body)
        color("burlywood") box_body();

    if (show_lid)
        translate([0,0,outer_h - lid_th])
            rotate([lid_open_angle,0,0])
                color("sienna") lid_block();

    if (show_false_bottom)
        color("saddlebrown") false_bottom();

    if (show_drawer)
        translate([wall_x + inner_len/2 - drawer_len/2 + drawer_clear,
                   0,
                   base_th])
            color("peru") drawer();

    // Lock vorne
    if (show_lock)
        color("gray") lock();
}

// ---------- Main ----------
if (!show_only_false_bottom_lasercut)
    assembly();
else
    false_bottom_lasercut();

