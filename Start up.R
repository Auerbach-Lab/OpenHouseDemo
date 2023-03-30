# Load packages -----------------------------------------------------------

# data manipulation
library(dplyr)

# graphing
library(hrbrthemes)
library(ggplot2)
library(ggtext)


# Load data ---------------------------------------------------------------
# needs to be in same folder as project
load("rat_data.Rdata")
# Prep rat data
rat_data = filter(rat_data, Intensity >= 40) %>% rename(reaction = Rxn, intensity = Intensity) %>%
  mutate(player = "Rat")

library(data.table)
Human_data <- fread("Human data.csv", header = TRUE) %>% mutate(player = "Human")



# Fake human data ---------------------------------------------------------

scores = tibble(player = c(0, 1, 2, 3),
                     quiet = c(150, 200, 210, 189),
                     mid = c(140, 132, 138, 178),
                     loud = c(110, 125, 132, 112))

