library(shiny)
library(shinyFeedback)
library(shinythemes)
library(shinyWidgets)
library(tidyverse, warn.conflicts = FALSE)
library(thematic)
library(stringr)
library(glue)
library(ggplot2)
library(hrbrthemes)
library(shinycustomloader)
library(shinyjs)
library(rlang)
library(serial, warn.conflicts = FALSE)

source("Start up.R")
source("Grapher.R")

arduino <- serialConnection(port="COM1", mode="9600,n,8,1")
open(arduino)

ui <- fluidPage(
  shinythemes::themeSelector(),
  useShinyFeedback(),
  useShinyjs(),
  theme = shinythemes::shinytheme("superhero"),
  #shinythemes::themeSelector(),
  titlePanel("Be A Lab Rat - Beckman Open House Demo", windowTitle = "Be A Lab Rat - Beckman Open House Demo"),
  textOutput("serial"),
  tableOutput("scoresTable"),
  plotOutput("plotPlayer1", height = "500px"),
  # actionButton("btnRefresh1", "Refresh 1", class = "btn btn-primary", style = "margin-top: 25px;", width = "150px"),
)

server <- function(input, output, session) {
  last_data <- reactiveVal(value = "Waiting for serial data...")

  scores <- reactiveVal(data.frame(player = as.character(seq(0,3)), loud = c(0,0,0,0), mid = c(0,0,0,0), quiet = c(0,0,0,0)))

  serialRead <- reactive({
    invalidateLater(50, session)
    read.serialConnection(arduino)
  })

  i = 1


  observeEvent(serialRead(), {
    if (!is_null(serialRead()) && !is_empty(serialRead()) && !is_na(serialRead()) && !(serialRead() == "")) {
      last_data(serialRead())
      a <- serialRead() %>% str_split(pattern = "[ =]")
      df <- data.frame(player=as.integer(a[[1]][2]), loud=as.integer(a[[1]][4]), mid=as.integer(a[[1]][6]), quiet=as.integer(a[[1]][8]))
      newscores <- scores()
      newscores <- newscores %>%
        left_join(df, by = "player") %>%
        mutate(loud = coalesce(loud.y, loud.x)) %>%
        mutate(mid = coalesce(mid.y, mid.x)) %>%
        mutate(quiet = coalesce(quiet.y, quiet.x)) %>%
        select(-loud.x, -loud.y, -mid.x, -mid.y, -quiet.x, -quiet.y)
      # I believe you want to do the following but I can't test it
      display_scores <- bind_rows(rat_score_table, newscores)
      scores(display_scores)
    } else if (serialRead() == "" & i == 1) {
      i <<- i+1
      # test
      newscores <- scores()
      display_scores <- bind_rows(rat_score_table, newscores)
      scores(display_scores)
    }
  })


  output$table <- renderDataTable(iris,
                                  options = list(
                                    pageLength = 5,
                                    initComplete = I("function(settings, json) {alert('Done.');}")
                                  )
  )

  output$scoresTable <- renderTable({scores()})

  output$serial <- renderText({last_data()})

  observeEvent(input$btnRefresh1, {

  })
}

options(shiny.host = "127.0.0.1") #setting this to an external IP address forces browser instead of rstudio window
shinyApp(ui, server)
