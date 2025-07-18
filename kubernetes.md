
Iniciar o minikube

```
minikube start
```


Verificar

```
minikube status
```

Carregar imagem do docker local no minikube

```
minikube image load pspd_mpi:latest
```

```
kubectl port-forward service/gateway 9999:9999
```

## Elastic Search

- [ECK - Elastic Cloud on Kubernetes](https://www.elastic.co/pt/downloads/elastic-cloud-kubernetes)

- [Deploy ElasticSearch](https://www.elastic.co/docs/deploy-manage/deploy/cloud-on-k8s/elasticsearch-deployment-quickstart)
- [Deploy Kibana](https://www.elastic.co/docs/deploy-manage/deploy/cloud-on-k8s/kibana-instance-quickstart)

```
$ curl -u "elastic:$PASSWORD" -k -X PUT "https://localhost:9200/pspd"
{"acknowledged":true,"shards_acknowledged":true,"index":"pspd"}%    
```

Gerar Api Key

```
curl -k -X POST "${ES_URL}/_bulk?pretty&pipeline=ent-search-generic-ingestion" \
  -H "Authorization: ApiKey "${API_KEY}"" \
  -H "Content-Type: application/json" \
  -d'
{ "index" : { "_index" : "pspd" } }
{ "duration": "60", "strategy": "example_strategy" }
'
```

## Gateway

## Spark

https://apache.github.io/spark-kubernetes-operator/

$ helm repo add spark https://apache.github.io/spark-kubernetes-operator
$ helm repo update
$ helm install spark spark/spark-kubernetes-operator


# kubectl apply -f https://raw.githubusercontent.com/apache/spark-kubernetes-operator/refs/tags/0.4.0/examples/prod-cluster-with-three-workers.yaml

kubectl apply -f https://raw.githubusercontent.com/apache/spark-kubernetes-operator/refs/heads/main/examples/cluster-with-template.yaml