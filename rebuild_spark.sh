docker build -t pspd_spark:latest Spark
minikube image load pspd_spark:latest
kubectl apply -f kubernetes/spark.yaml