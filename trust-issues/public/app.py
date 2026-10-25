
from flask import Flask, render_template, request
import os
import subprocess


app = Flask(__name__)


@app.get("/")
def index():
    return render_template("index.html")

@app.post("/")
def connect():
    ip = request.form["ip"]
    port = request.form["port"]
    
    flag = os.environ["FLAG"]

    process = subprocess.run(["./client", ip, port, flag])

    if process.returncode == 0:
        result = "Flag transmitted successfully!"
    else:
        result = "A communication error occurred."
    
    return {
        "result": result
    }
