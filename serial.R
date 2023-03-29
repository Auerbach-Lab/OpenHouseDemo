library(tidyverse, warn.conflicts = FALSE)
library(serial, warn.conflicts = FALSE)
library(magrittr, warn.conflicts = FALSE)



# SERIAL CONNECTION
  # listPorts()
  myArduino <-  serialConnection(
    port = "COM7",
    mode = "9600,n,8,1" ,
    buffering = "none",
    newline = TRUE,
    eof = "",
    translation = "cr",
    handshake = "none",
    buffersize = 4096
  )

# OPEN AND TESTING THE CONNECTION
  while (!isOpen(myArduino)) open(myArduino)

# READ MAPPED DATA SENT FROM MY ARDUINO
  dataFromArduino <- tibble(
    capture.output(cat(read.serialConnection(myArduino, n = 0)))
  )
# SELECT FIRST NINE ROWS, ASSIGN VALUES TO THEIR LEDS AND RENAME FIRST COLUMN
  dataFromArduino %>%
    slice_head(n = 8)










  #close(myArduino)
