Grapher <- function(scores) {
 # Adjust player data ------------------------------------------------------
  player_data =
    scores %>%
    # remove rat scores
    filter(str_detect(player, pattern = "Rat", negate = TRUE)) %>%
    # artificially assign an intensity value
    rename(`40` = quiet, `60` = mid, `90` = loud) %>%
    # convert to long format
    tidyr::gather(key = "intensity", value = "reaction", num_range("", 10:90)) %>%
    mutate(intensity = as.numeric(intensity))

  # Make Player graph --------------------------------------------------------
  Player_graph =
    player_data %>%
    # may need to convert from low, medium, high to number using mutate
    ggplot(aes(x = intensity, y = reaction, color = as.factor(player), group = as.factor(player))) +
    stat_summary(fun = mean, geom = "line", linewidth = 2) +
    stat_summary(fun = mean, geom = "point", size = 5)


  # Add rat & human average data ---------------------------------------------
  Final_graph =
    Player_graph +
      # Rat data
      geom_smooth(data = rat_data,
                  se = FALSE, na.rm = TRUE, linewidth = 2, linetype = "dotdash",
                  method = "lm", formula = y ~ x
      ) +
      geom_smooth(data = Human_data,
                  se = FALSE, na.rm = TRUE, linewidth = 2, linetype = "dotdash",
                  method = "lm", formula = y ~ x
      ) +
    scale_color_manual(values = c(
      "1" = "mediumorchid", "2" = "grey99", "3" = "seagreen3", "0" = "royalblue", "Rat" = "firebrick3", "Human" = "gold"
    )) +
    labs(x = "Loudness\n(Intensity, dB)",
         y = "Speed\n(Average reaction time, ms)",
         color = "Player") +
    scale_x_continuous(breaks = seq(-50, 90, by = 10)) +
    labs(title = "<span style='color:#CD2626;'>Rat</span> v. <span style='color:#FFD700;'>Human</span>: *the ultimate hearing test*") +
    annotate(geom = "text", x = 38, y = c(129, max(player_data$reaction, rat_data$reaction)),
             label = c("Fast", "Slow" ), size = 8) +
    annotate(geom = "text", x = c(42,88), y = 92,
             label = c("Quiet", "Loud" ), size = 8) +
    theme_ft_rc() +
    theme(
      # plot.title = element_text(size = 32),
      plot.title = element_markdown(size = 32)
    )


  print(Final_graph)
  return(Final_graph)
}
