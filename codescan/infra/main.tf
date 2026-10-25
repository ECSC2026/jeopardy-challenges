terraform {
  required_version = ">= 1.5.0"

  required_providers {
    github = {
      source  = "integrations/github"
      version = "~> 6.13"
    }
  }
}

locals {
  team_ids = toset([
    "Albania",
    "Austria",
    "Belgium",
    "Bulgaria",
    "Croatia",
    "Cyprus",
    "Czechia",
    "Denmark",
    "Estonia",
    "Finland",
    "France",
    "Germany",
    "Greece",
    "Hungary",
    "Iceland",
    "Ireland",
    "Italy",
    "Latvia",
    "Liechtenstein",
    "Lithuania",
    "Luxembourg",
    "Malta",
    "Netherlands",
    "Norway",
    "Poland",
    "Portugal",
    "Romania",
    "Serbia",
    "Slovakia",
    "Slovenia",
    "Spain",
    "Sweden",
    "Switzerland",
    "Ukraine",
    "United Kingdom",
    "Armenia",
    "Australia",
    "Costa Rica",
    "Canada",
    "Kosovo",
    "Singapore",
    "United States",
    "2517",
    "67",
    "79",
  ])

  repository_names = {
    for team_id in local.team_ids :
    team_id => "codescan-team-${replace(lower(team_id), " ", "-")}"
  }
}

provider "github" {
  owner = "ecsc-2026-codescan"
}

resource "github_organization_settings" "codescan" {
  billing_email                           = "moritz.sanft@outlook.de"
  default_repository_permission           = "none"
  members_can_create_repositories         = false
  members_can_create_private_repositories = false
  members_can_create_public_repositories  = false
  members_can_fork_private_repositories   = true
}

resource "github_team" "codescan" {
  for_each = local.team_ids

  name                 = "codescan-team-${each.value}"
  description          = "Competitors assigned to Codescan team ${each.value}"
  privacy              = "secret"
  notification_setting = "notifications_disabled"
}

resource "github_repository" "template" {
  name        = "codescan-template"
  description = "Template for Codescan team repositories"
  visibility  = "private"
  is_template = true

  has_issues      = false
  has_projects    = false
  has_wiki        = false
  has_discussions = false

  depends_on = [github_organization_settings.codescan]
}

resource "github_repository_file" "main_c" {
  repository          = github_repository.template.name
  file                = "main.c"
  commit_message      = "Initial commit"
  overwrite_on_create = true
  content             = <<-EOF
    #include <stdio.h>

    int main(void) {
        puts("Hello, world!");
        return 0;
    }
  EOF
}

resource "github_repository" "upstream" {
  for_each = local.team_ids

  name        = local.repository_names[each.value]
  description = "Private Codescan upstream for team ${each.value}"
  visibility  = "private"

  allow_forking   = true
  has_issues      = false
  has_projects    = false
  has_wiki        = false
  has_discussions = false

  template {
    owner      = "ecsc-2026-codescan"
    repository = github_repository.template.name
  }

  depends_on = [github_repository_file.main_c]
}

resource "github_team_repository" "upstream" {
  for_each = local.team_ids

  team_id    = github_team.codescan[each.value].id
  repository = github_repository.upstream[each.value].name
  permission = "pull"
}
