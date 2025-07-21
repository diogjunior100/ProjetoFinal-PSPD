
Iniciar o minikube

```
minikube start --cpus=12 --memory=10000 --addons=metrics,dns

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

kubectl create -f https://download.elastic.co/downloads/eck/3.0.0/crds.yaml
kubectl apply -f https://download.elastic.co/downloads/eck/3.0.0/operator.yaml


kubectl get secret quickstart-es-elastic-user -o go-template='{{.data.elastic | base64decode}}'

- [Deploy ElasticSearch](https://www.elastic.co/docs/deploy-manage/deploy/cloud-on-k8s/elasticsearch-deployment-quickstart)

```
cat <<EOF | kubectl apply -f -
apiVersion: elasticsearch.k8s.elastic.co/v1
kind: Elasticsearch
metadata:
  name: quickstart
spec:
  version: 8.16.1
  nodeSets:
  - name: default
    count: 1
    config:
      node.store.allow_mmap: false
EOF
```

- [Deploy Kibana](https://www.elastic.co/docs/deploy-manage/deploy/cloud-on-k8s/kibana-instance-quickstart)

```
cat <<EOF | kubectl apply -f -
apiVersion: kibana.k8s.elastic.co/v1
kind: Kibana
metadata:
  name: quickstart
spec:
  version: 8.16.1
  count: 1
  elasticsearchRef:
    name: quickstart
EOF
```

```
$ curl -u "elastic:$PASSWORD" -k -X PUT "https://localhost:9200/pspd"
{"acknowledged":true,"shards_acknowledged":true,"index":"pspd"}%    
```

Gerar Api Key

```
curl -k -X POST "https://localhost:9200/_bulk?pretty&pipeline=ent-search-generic-ingestion" \
  -H "Authorization: ApiKey "bGpyVEk1Z0JxMUgzdUM3WElmSGg6VEc0RzU5NmFSWFdROGhPaDcwWWJOUQ=="" \
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
