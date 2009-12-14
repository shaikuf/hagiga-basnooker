%initialize device
vid = videoinput('winvideo', 1, 'RGB24_1600x1200')

%angular opening calculation
dist  = 235 %cm
width = 220 %cm
phi = 2*atan(110/235) %radians
phi_degrees = 360*phi/(2*pi) %degrees

preview(vid)
