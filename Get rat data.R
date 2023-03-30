rat_data =
  # Taken from TTS SD rats
  Rxn_table %>%
  # select base base (baseline BBN with no background at 300ms)
  filter(HL_state %in% c("baseline") & Frequency == 0 &
           BG_Intensity == "None" & Duration == "300")

save(rat_data, file = "rat_data.Rdata", ascii = FALSE, compress = FALSE)
