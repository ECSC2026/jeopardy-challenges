FROM python:3.14-slim@sha256:cad9a2c871761c413caa6fdd6441c783451e740a48aaeba60ae62a8b53525ef6

ENV FLAG="ECSC{why_sh0uld_1_tr4nsm1t_a_n0nc3_wh3n_1_c4n_use_th3_k3y}"

WORKDIR /app

COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt \
    && groupadd --system challenge \
    && useradd --system --gid challenge --create-home challenge

COPY --chown=challenge:challenge app.py ./
COPY --chown=challenge:challenge templates ./templates
COPY --chown=challenge:challenge static ./static

USER challenge

EXPOSE 1337
ENTRYPOINT ["flask", "--app", "app", "run", "--host=0.0.0.0", "--port=1337"]
