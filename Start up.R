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
rat_data = ungroup(rat_data) %>% filter(intensity >= 40) %>%
  mutate(player = "Rat")

# build rat_score_table
# Note hard coded based on fastest and slowest rats with complete data
rat_score_table =
  filter(rat_data, rat_ID %in% c(15, 20) & intensity %in% c(40, 60, 90)) %>%
  select(rat_ID, rat_name, intensity, reaction) %>%
  tidyr::spread(intensity, reaction) %>%
  # sort so slowest is on bottom
  arrange(desc(`40`)) %>% mutate(player = c("Rat (Slowest)", "Rat (Fastest)"), .before = rat_ID) %>%
  rename(quiet = `40`, mid = `60`, loud = `90`) %>%
  select(-c(rat_ID, rat_name)) %>%
  select(player, loud, mid, quiet)

library(data.table)
if (file.exists("human_average.csv")) {
  Human_data <- fread("human_average.csv", header = TRUE)
  } else {
    Human_data <- fread("Human data.csv", header = TRUE) %>% mutate(player = "Human")
    fwrite(Human_data, "human_average.csv")
  }


# Necessary functions -----------------------------------------------------

Update_human_averages <- function(single_player_data) {
  # Add data to averages in memory at bottom of table
  Human_data = bind_rows(Human_data, mutate(single_player_data, player = "Human"))
  # pop off new row with all necessary columns for appending to existing table
  new_row = tail(Human_data, n = 1)

  # Add new data to saved table
  data.table::fwrite(new_row, file = "human_average.csv", append = TRUE)
}


#
#
# # Fake human data ---------------------------------------------------------
#
# scores = tibble(player = c(0, 1, 2, 3),
#                      quiet = c(150, 200, 210, 189),
#                      mid = c(140, 132, 138, 178),
#                      loud = c(110, 125, 132, 112))

