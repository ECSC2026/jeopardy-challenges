FROM mcr.microsoft.com/dotnet/sdk:10.0 AS build

WORKDIR /src

COPY i-like-trains.csproj .
RUN dotnet restore

COPY . .
RUN dotnet publish -c Release -o /app/publish

FROM mcr.microsoft.com/dotnet/aspnet:10.0

WORKDIR /app

RUN apt-get update && \
    apt-get install -y wget unzip && \
    rm -rf /var/lib/apt/lists/* && \
    mkdir /data && \
    chown nobody:nogroup /data && \
    wget 'https://download.gtfs.de/germany/fv_free/latest.zip' -O /data/gtfs.zip && \
    unzip /data/gtfs.zip -d /data/gtfs && \
    rm /data/gtfs.zip && \
    mkdir /data/tools /data/uploads && \
    chown nobody:nogroup /data/tools /data/uploads

COPY --from=ghcr.io/pspaul/ctf /readflag /readflag
COPY --chown=root:root --chmod=400 flag.txt /flag.txt
COPY init.sql .
COPY --from=build /app/publish .

USER nobody

ENTRYPOINT ["dotnet", "i-like-trains.dll"]
