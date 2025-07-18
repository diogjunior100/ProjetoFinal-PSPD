docker build -t pspd_spark4-1:latest Spark
minikube image load pspd_spark4-1:latest
kubectl apply -f kubernetes/spark.yaml