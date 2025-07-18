docker build -t pspd_spark4-5:latest Spark
minikube image load pspd_spark4-5:latest
kubectl apply -f kubernetes/spark.yaml