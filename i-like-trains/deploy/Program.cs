using System.ComponentModel.DataAnnotations;
using System.Diagnostics;
using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;
using Microsoft.AspNetCore.Authentication.JwtBearer;
using Microsoft.Data.Sqlite;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using NLog;
using NLog.Layouts;
using NLog.Web;

var builder = WebApplication.CreateBuilder(args);

builder.Logging.ClearProviders();
builder.Host.UseNLog();

var logger = LogManager.Setup()
    .SetupExtensions(s => s.RegisterLayoutRenderer("msg",
        evt => new SimpleLayout(evt.FormattedMessage).Render(evt)))
    .LoadConfigurationFromAppSettings()
    .GetCurrentClassLogger();

var connectionString = builder.Configuration.GetConnectionString("Default")!;

await using var connection = new SqliteConnection(connectionString);
await connection.OpenAsync();
await using var cmd = connection.CreateCommand();
cmd.CommandText = await File.ReadAllTextAsync("init.sql");
await cmd.ExecuteNonQueryAsync();
connection.Close();

var jwtSection = builder.Configuration.GetSection("Jwt");
var jwtKey = jwtSection["Key"]!;
var jwtIssuer = jwtSection["Issuer"]!;
var jwtAudience = jwtSection["Audience"]!;
var jwtExpiryHours = jwtSection.GetValue<int>("ExpiryHours", 1);

builder.Services
    .AddAuthentication(JwtBearerDefaults.AuthenticationScheme)
    .AddJwtBearer(options =>
    {
        options.TokenValidationParameters = new TokenValidationParameters
        {
            ValidateIssuer = true,
            ValidateAudience = true,
            ValidateLifetime = true,
            ValidateIssuerSigningKey = true,
            ValidIssuer = jwtIssuer,
            ValidAudience = jwtAudience,
            IssuerSigningKey = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(jwtKey)),
        };
    });

builder.Services.AddAuthorization();
builder.Services.AddDbContext<AppDbContext>(options => options.UseSqlite(connectionString));

var app = builder.Build();

app.UseAuthentication();
app.UseAuthorization();

var dataDir = Environment.GetEnvironmentVariable("DATA_DIR") ?? AppContext.BaseDirectory;
var gtfsDir = Path.Combine(dataDir, "gtfs");
var toolsDir = Path.Combine(dataDir, "tools");
Directory.CreateDirectory(toolsDir);

var indexHtmlPath = Path.Combine(AppContext.BaseDirectory, "index.html");

app.MapGet("/", async (HttpRequest request) =>
{
    if (File.Exists(indexHtmlPath))
        return Results.Content(await File.ReadAllTextAsync(indexHtmlPath), "text/html; charset=utf-8");
    return Results.Content("i like trains", "text/plain");
});

app.MapPost("/login", (LoginRequest req, IConfiguration config, AppDbContext db) =>
{
    var username = req.Username;
    var password = req.Password;
    var user = db.Users.SingleOrDefault(u => u.Username == username);
    if (user is null || user.Password is null || user.Password != password)
    {
        logger.Warn($"Failed login attempt for user {username}");
        return Results.Unauthorized();
    }
    logger.Info("Successful login for user {username}", username);

    var key = new SymmetricSecurityKey(Encoding.UTF8.GetBytes(jwtKey));
    var token = new JwtSecurityToken(
        issuer: jwtIssuer,
        audience: jwtAudience,
        claims: [new Claim(ClaimTypes.Name, username)],
        expires: DateTime.UtcNow.AddHours(jwtExpiryHours),
        signingCredentials: new SigningCredentials(key, SecurityAlgorithms.HmacSha256)
    );

    return Results.Ok(new { token = new JwtSecurityTokenHandler().WriteToken(token) });
});

app.MapPost("/query", async (HttpRequest request, ClaimsPrincipal user, AppDbContext db) =>
{
    using var reader = new StreamReader(request.Body);
    var sql = await reader.ReadToEndAsync();
    logger.Info("Debug query from {User}: {Sql}", user.Identity?.Name, sql);

    var connection = db.Database.GetDbConnection();
    if (connection.State != System.Data.ConnectionState.Open)
        await connection.OpenAsync();
    await using var cmd = connection.CreateCommand();
    cmd.CommandText = sql;

    var rows = new List<Dictionary<string, object?>>();
    await using var dbReader = await cmd.ExecuteReaderAsync();
    while (await dbReader.ReadAsync())
    {
        var row = new Dictionary<string, object?>();
        for (var i = 0; i < dbReader.FieldCount; i++)
            row[dbReader.GetName(i)] = dbReader.IsDBNull(i) ? null : dbReader.GetValue(i);
        rows.Add(row);
    }

    return Results.Ok(rows);
}).RequireAuthorization();

app.MapPost("/install", async (HttpRequest request, IConfiguration config, AppDbContext _context) =>
{
    var checksums = config.GetSection("ThirdPartyChecksums").Get<Dictionary<string, string>>() ?? new Dictionary<string, string>();

    foreach (var resource in await _context.ThirdPartyResources.ToListAsync())
    {
        logger.Info("Resource: {Name} URL: {Url}", resource.Name, resource.Url);
        var name = resource.Name;
        var url = resource.Url;
        if (!checksums.TryGetValue(name, out var checksum))
        {
            logger.Warn("No trusted checksum configured for resource {Name}, skipping", name);
            return Results.Problem($"No trusted checksum configured for resource {name}");
        }
        logger.Info("Resource: {Name} URL: {Url} Checksum: {Checksum}", name, url, checksum);

        var content = await Download(url);
        if (!await VerifyChecksum(content, checksum))
        {
            logger.Warn("Checksum verification failed for resource {Name} from {Url}", name, url);
            return Results.Problem($"Checksum verification failed for resource {name}");
        }
        
        var destPath = Path.Combine(toolsDir, name);
        await File.WriteAllBytesAsync(destPath, content);
        File.SetUnixFileMode(destPath, (UnixFileMode)System.Convert.ToInt32("755", 8));

        logger.Info("Successfully downloaded and verified resource {Name} from {Url} to {DestPath}", name, url, destPath);
    }

    return Results.Ok();
}).RequireAuthorization();

app.MapGet("/api/agencies", async () =>
{
    var output = await QueryCSV(Path.Combine(gtfsDir, "agency.txt"), "[.[] | {\"name\": .agency_name, \"url\": .agency_url}]");
    return Results.Content(output, "application/json; charset=utf-8");
});

app.MapGet("/api/stations", async () =>
{
    var output = await QueryCSV(Path.Combine(gtfsDir, "stops.txt"), "[.[] | {\"name\": .stop_name, \"lat\": .stop_lat, \"lon\": .stop_lon}]");
    return Results.Content(output, "application/json; charset=utf-8");
});

app.MapGet("/api/lines/ice", async () =>
{
    var output = await QueryCSV(Path.Combine(gtfsDir, "routes.txt"), "[.[] | select(.route_short_name | test(\"^ICE\")) | .route_short_name] | unique");
    return Results.Content(output, "application/json; charset=utf-8");
});

app.MapGet("/api/destinations", async () =>
{
    var output = await QueryCSV(Path.Combine(gtfsDir, "stop_times.txt"), "[.[] | .stop_headsign | select(. != \"\")] | unique");
    return Results.Content(output, "application/json; charset=utf-8");
});

app.Run();

async Task<string> QueryCSV(string file, string query)
{
    var yqPath = Path.Combine(toolsDir, "yq");
    if (!File.Exists(yqPath))
    {
        logger.Warn("yq not found at {Path}", yqPath);
        throw new Exception("yq not available, pls /install 3rd-party tools");
    }

    var process = new Process
    {
        StartInfo = new ProcessStartInfo
        {
            FileName = yqPath,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
        }
    };
    process.StartInfo.ArgumentList.Add("-p=csv");
    process.StartInfo.ArgumentList.Add("-o=json");
    process.StartInfo.ArgumentList.Add(query);
    process.StartInfo.ArgumentList.Add(file);
    process.Start();
    var output = await process.StandardOutput.ReadToEndAsync();
    var error = await process.StandardError.ReadToEndAsync();
    await process.WaitForExitAsync();

    if (process.ExitCode != 0)
    {
        logger.Warn("Query failed: {Error}", error);
        throw new Exception($"Query failed: {error}");
    }
    return output;
}

async Task<byte[]> Download(string url)
{
    using var httpClient = new HttpClient();
    var response = await httpClient.GetAsync(url);
    if (!response.IsSuccessStatusCode)
    {
        logger.Warn("Failed to download {Url}: {StatusCode}", url, response.StatusCode);
        throw new Exception($"Failed to download {url}");
    }
    return await response.Content.ReadAsByteArrayAsync();
}

static async Task<bool> VerifyChecksum(byte[] content, string expectedChecksum)
{
    using var sha256 = System.Security.Cryptography.SHA256.Create();
    await using var stream = new MemoryStream(content);
    var hashBytes = await sha256.ComputeHashAsync(stream);
    var actualChecksum = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();
    return string.Equals(actualChecksum, expectedChecksum, StringComparison.OrdinalIgnoreCase);
}

record LoginRequest(string Username, string Password);

public record User(int Id, string Username, string Password);

public record ThirdPartyResource([property: Key] string Name, string Url);

public class AppDbContext : DbContext
{
    public AppDbContext(DbContextOptions<AppDbContext> options) : base(options) { }
    public DbSet<ThirdPartyResource> ThirdPartyResources { get; set; }
    public DbSet<User> Users { get; set; }
}
