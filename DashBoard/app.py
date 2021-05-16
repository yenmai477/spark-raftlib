from flask import Flask, jsonify, request
from flask import render_template
import ast

app = Flask(__name__)

labels = []
values_counts = []


@app.route("/")
def get_chart_page():
    global labels, values_counts
    labels = []
    values_counts = []

    # labels = ["#bunkerbitch", "#bunkerboy", "#fakepresident", "#trumpresignnow", "#resignnowtrump", "#bunkerbaby", "#protests2020", "#blacklivesmatter", "#anonymous", "#maga"]
    # values_counts = ["155","103","47","23","23","31","17","20","14","34"]
    return render_template('chart.html', values_counts=values_counts, labels=labels)


@app.route('/refreshData')
def refresh_graph_data():
    global labels, values_counts

    return jsonify(sLabel=labels, sData_counts=values_counts)


@app.route('/updateData', methods=['POST'])
def update_data_post():
    global labels, values_counts
    if not request.form:
        return "error", 400
    labels = ast.literal_eval(request.form['label'])
    values_counts = ast.literal_eval(request.form['data_counts'])

    # print("labels received: " + str(labels))
    # print("data received: " + str(values_counts))
    return "success", 201


if __name__ == "__main__":
    app.run(host='localhost', port=5001)
