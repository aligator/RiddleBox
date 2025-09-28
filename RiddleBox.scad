// Creative Deco wooden box with false bottom and secret drawer
// Outer size: 300 x 200 x 140 mm

// ---------- Parameters ----------
// Master dimensions
outer_len = 300;     // outer length (X, mm)
outer_w   = 200;     // outer width (Y, mm)
outer_h   = 140;     // outer height (Z, mm)

// Wall thickness
wall_x  = 12.5;   // left/right walls (X)
wall_y = 12.5;   // front/back walls (Y)

// Base and lid
base_th = 10;        // bottom thickness
lid_th  = 32;        // lid block thickness

// Hidden compartment
hidden_h         = 20;   // hidden drawer height (Z)
false_bottom_th  = 5;    // false bottom thickness
drawer_len       = 120;  // drawer length in X direction (< inner_len)
drawer_clear     = 1.0;  // clearance for drawer (mm)
drawer_power_hole_d = 10;

// Derived dimensions
inner_len = outer_len - 2*wall_x;
inner_w   = outer_w   - 2*wall_y;
inner_h   = outer_h - base_th - lid_th;

lid_cutout = lid_th - 6; // hollow inside lid, leave 6 mm material
lid_clear  = 0.6;        // clearance for lid/lip

// Display
lid_open_angle = 100; // degrees, 0 = closed, 90 = open

// ---------- Feature toggles ----------
show_box_body     = true;
show_lid          = true;
show_false_bottom = true;
show_drawer       = true;
show_drawer_cut   = true;

c0=0.01+0;

// ---------- Modules ----------
module box_body(){
    difference(){
        // Outer shell without lid
        cube([outer_len, outer_w, outer_h - lid_th], center=false);

        // Visible compartment above false bottom
        translate([wall_x, wall_y, base_th])
            cube([inner_len, inner_w, inner_h+c0], center=false);

        // Rear cutout for drawer access (goes through the back wall)
        if (show_drawer_cut)
            translate([wall_x + inner_len/2 - drawer_len/2,-c0, base_th])
                cube([drawer_len, wall_y+2+c0, hidden_h], center=false);
    }
}

module lid_block(){
    difference(){
        // Lid block
        cube([outer_len, outer_w, lid_th], center=false);

        // Hollow inside lid
        translate([wall_x-lid_clear/2, wall_y-lid_clear/2, -c0])
            cube([inner_len+lid_clear, inner_w+lid_clear, lid_cutout+c0], center=false);
    }
}

module drawer(){
    drawer_wall = 3;  // wall thickness of drawer
    difference(){
        // Outer size = hidden space minus clearance
        translate([0, 0, 0])
        cube([drawer_len-2*drawer_clear,
              inner_w + wall_y,
              hidden_h-drawer_clear], center=false);

        // Inner hollow, top open
        translate([drawer_wall, drawer_wall, drawer_wall])
            cube([drawer_len-2*drawer_clear-2*drawer_wall,
                  inner_w+wall_y - drawer_wall*2,
                  hidden_h-drawer_clear], center=false);
        
        translate([(drawer_len-drawer_clear*2)/2, drawer_wall+c0, (hidden_h-drawer_clear)/2])
        rotate([90, 0, 0])
        cylinder(d=drawer_power_hole_d, h=drawer_wall+2*c0);
    }
}

module false_bottom(){
    // Plate above the hidden drawer
    translate([wall_x, wall_y, base_th + hidden_h])
        cube([inner_len, inner_w, false_bottom_th], center=false);
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
        translate([wall_x + inner_len/2 - drawer_len/2+drawer_clear, 0, base_th])
            color("peru") drawer();
}

// ---------- Main ----------
assembly();
