# In /home/ramin/Documents/learningCodeForESP32/WifiStationExample/main/CMakeLists.txt

idf_component_register(SRCS "main.c"
                           "myWifi.c"  # <-- Add this line
                   INCLUDE_DIRS ".")
