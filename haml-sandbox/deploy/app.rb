require "haml"
require "sinatra/base"

class HamlSandbox < Sinatra::Base
  FORBIDDEN = ["=", "-", "~", "%", "\#{", "{", "[", ":ruby", ":erb"].freeze

  set :bind, "0.0.0.0"
  set :port, 31_337
  # Every session is served under its own hostname, so Sinatra's
  # development-mode host allowlist (localhost/.test only) has to go.
  set :host_authorization, permitted_hosts: []
  set :raise_errors, false
  set :show_exceptions, false

  get "/" do
    File.read("index.html")
  end

  post "/render" do
    template = params.fetch("template", "").to_s
    lowertext = template.downcase
    blocked = FORBIDDEN.find { |token| lowertext.include?(token) }
    halt 400, "blocked token: '#{blocked}'\n" if blocked

    Haml::Template.new { template }.render
  rescue StandardError => e
    status 500
    "#{e.class}: #{e.message}\n"
  end

  run! if app_file == $PROGRAM_NAME
end
