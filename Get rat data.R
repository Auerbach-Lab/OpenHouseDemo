rat_data_TTS =
  # Taken from TTS SD rats
  Rxn_table %>%
  # select base base (baseline BBN with no background at 300ms)
  filter(HL_state %in% c("baseline") & Frequency == 0 &
           BG_Intensity == "None" & Duration == "300") %>%
  mutate(player = "Rat") %>%
  rename(reaction = Rxn, intensity = Intensity)

rat_data_WT_LE =
  # Taken from Fmr1 & Tsc2 SD rats
  Rxn_table %>%
  # select base base (baseline BBN with no background at 300ms)
  filter(genotype == "WT" & `Freq (kHz)` == 0 & `Dur (ms)` == "300" & detail == "Alone") %>%
  mutate(player = "Rat") %>%
  rename(Frequency = `Freq (kHz)`, Duration = `Dur (ms)`, intensity = `Inten (dB)`, reaction = Rxn)

rat_data = bind_rows(rat_data_TTS, rat_data_WT_LE)


save(rat_data, file = "rat_data.Rdata", ascii = FALSE, compress = FALSE)
