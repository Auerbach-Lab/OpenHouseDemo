# Adjust player data ------------------------------------------------------
player_data =
  scores %>%
  # artificially assign an intensity value
  rename(`40` = quiet, `60` = mid, `90` = loud) %>%
  # convert to long format
  tidyr::gather(key = "intensity", value = "reaction", num_range("", 10:90)) %>%
  mutate(intensity = as.numeric(intensity))


# Rat Graph ---------------------------------------------------------------
# Make 1st if wanted underneath

# Make Human graph --------------------------------------------------------
Human_graph =
  player_data %>%
  # may need to convert from low, medium, high to number using mutate
  ggplot(aes(x = intensity, y = reaction, color = as.factor(player), group = as.factor(player))) +
  stat_summary(fun = mean, geom = "line", linewidth = 2) +
  stat_summary(fun = mean, geom = "point", size = 5)

# print(Human_graph)


# Add rat data ------------------------------------------------------------
Final_graph =
  Human_graph +
    # Rat data
    geom_smooth(data = rat_data,
                se = FALSE, na.rm = TRUE, linewidth = 2, linetype = "longdash",
                method = "lm", formula = y ~ x
    ) +
    geom_smooth(data = Human_data,
                se = FALSE, na.rm = TRUE, linewidth = 2, linetype = "dotdash",
                method = "lm", formula = y ~ x
    ) +
  scale_color_manual(values = c(
    "1" = "mediumorchid", "2" = "wheat3", "3" = "seagreen3", "0" = "royalblue", "Rat" = "black", "Human" = "darkgrey"
  )) +
  labs(x = "Loudness\n(Intensity, dB)",
       y = "Speed\n(Average reaction time, ms)",
       color = "Player") +
  scale_x_continuous(breaks = seq(-50, 90, by = 10)) +
  labs(title = "Rat v. human: the ultimate hearing test") +
  annotate(geom = "text", x = 39, y = c(105, max(player_data$reaction, rat_data$reaction)),
           label = c("Fast", "Slow" ), size = 5) +
  annotate(geom = "text", x = c(39,91), y = 97,
           label = c("Quiet", "Loud" ), size = 5) +
  theme_ft_rc()
  # theme_classic() +
  # theme(legend.position = "right",
  #       # legend.position = c(0.9, 0.8),
  #       legend.background=element_blank(),
  #       plot.title = element_text(hjust = 0.5),
  #       panel.grid.major.x = element_line(color = rgb(235, 235, 235, 255, maxColorValue = 255)),
  #       panel.grid.major.y = element_line(color = rgb(235, 235, 235, 255, maxColorValue = 255)),
  # )

print(Final_graph)


# Keep running total of human data ----------------------------------------

Human_data = bind_rows(Human_data, player_data %>%
                                          mutate(player = "Human")
                       )

data.table::fwrite(Human_data, file = "human_average.csv")
