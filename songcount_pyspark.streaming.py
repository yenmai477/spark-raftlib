# coding=utf8

# Tài liệu tham khảo:


import sys

from pyspark import SparkContext
from pyspark.streaming import StreamingContext
from pyspark.sql import Row, SQLContext
import sys
import requests
import re


def aggregate_songs_count(new_values, total_sum):
    total_sum = total_sum if total_sum else 0
    return sum(new_values) + total_sum


def get_sql_context_instance(spark_context):
    if ('sqlContextSingletonInstance' not in globals()):
        globals()['sqlContextSingletonInstance'] = SQLContext(spark_context)
    return globals()['sqlContextSingletonInstance']


def process_rdd(time, rdd):
    # print("----------- %s -----------" % str(time))
    try:
        # Get spark sql singleton context from the current context
        sql_context = get_sql_context_instance(rdd.context)

        # convert the RDD to Row RDD
        row_rdd = rdd.map(lambda w: Row(song=w[0], song_count=w[1]))

        # create a DF from the Row RDD
        songs_df = sql_context.createDataFrame(row_rdd)

        # Register the dataframe as table
        songs_df.registerTempTable("songs")

        # get the top 10 songs from the table using SQL and print them
        song_counts_df = sql_context.sql(
            "select song, song_count from songs order by song_count desc limit 10")

        # Show top 10 songs but comment to easy see result in terminal
        song_counts_df.show()

        # call this method to prepare top 10 songs DF and send them
        send_df_to_dashboard(song_counts_df)

    except:
        e = sys.exc_info()[0]
        print("Error: %s" % str(e))


def send_df_to_dashboard(df):

    # extract the songs from dataframe and convert them into array
    top_songs = [str(t.song) for t in df.select("song").collect()]

    # extract the counts from dataframe and convert them into array
    counts = [c.song_count for c in df.select("song_count").collect()]

    # initialize and send the data through REST API
    url = 'http://localhost:5001/updateData'
    request_data = {'label': str(top_songs), 'data_counts': str(counts)}
    response = requests.post(url, data=request_data)


if __name__ == "__main__":
    # Tạo một local StreamingContext với 2 luồng và batch sẽ được gọi mỗi 2 giây
    sc = SparkContext("local[2]", "NetworkWordCount")

    ssc = StreamingContext(sc, 2)

    ##############################################

    # setting a checkpoint to allow RDD recovery
    ssc.checkpoint("checkpoint")

    # Tạo một DStream chứa các line liên kết đến localhost:9999
    lines = ssc.socketTextStream("localhost", 9999)

    # In ra số bài hát trong mỗi batch của spark
    lines.count().map(lambda x: 'Lines in this batch: %s' % x).pprint()

    # Tách các tên bài hát trong từng batch ra
    counts = lines.flatMap(lambda x: x.split('\n'))

    # Pha map
    counts = counts.map(lambda x: (x, 1))

    # Pha reduce
    # counts = counts.reduceByKey(lambda a, b: a + b)
    counts = counts.reduceByKey(
        lambda a, b: a + b).updateStateByKey(aggregate_songs_count)

    counts.foreachRDD(process_rdd)

    # In kết quả ra màn hình
    counts.pprint(9999)  # Sẽ in ra tối đa số dòng truyền vào, mặc định là 10

    ##############################################

    ssc.start()             # Bắt đầu tính toán
    ssc.awaitTermination()  # Chờ cho đến khi kết thúc
