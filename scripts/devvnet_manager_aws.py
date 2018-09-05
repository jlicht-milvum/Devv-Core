import yaml
import argparse
import sys
import os
import subprocess
import time
import boto3

validator_json_string_o = """
{
    "cluster": "$cluster",
    "taskDefinition": "arn:aws:ecs:$region:682078287735:task-definition/devv-validator-00",
    "overrides": {
	"containerOverrides": [
	    {
		"name": "devv-bin-full-validator",
		"command": [
		    "devcash",
		    "--shard-index", "1",
		    "--node-index", "0",
		    "--config", "/etc/devv/validator.conf",
		    "--config", "/etc/devv/default_pass.conf",
		    "--bind-endpoint", "tcp://*:55601",
		    "--host-list", "tcp://10.0.0.191:55601",
		    "--host-list", "tcp://10.0.0.242:55601",
		    "--host-list", "tcp://10.0.0.150:55602"
		]
	    }
	],
	"taskRoleArn": "arn:aws:iam::682078287735:role/ecsTaskExecutionRole",
	"executionRoleArn": "arn:aws:iam::682078287735:role/ecsTaskExecutionRole"
    },
    "containerInstances": [
        "ce4c55f8-e53c-4051-a890-bc87f36ad7a7"
    ]
}
"""
validator_json_string = """
{
    "cluster": "$cluster",
    "taskDefinition": "$task_definition",
    "overrides": {
	"containerOverrides": [
	    {
		"name": "$container_name",
		"command": [ $command ]
	    }
	],
	"taskRoleArn": "$task_role_arn",
	"executionRoleArn": "$execution_role_arn"
    },
    "containerInstances": [
        "$container_instance"
    ]
}
"""

def get_task_list(session):
    ecs = session.client('ecs')
    tasks = ecs.list_tasks(cluster = 'devv-test-cluster-01')['taskArns']
    return tasks

def get_task(task_num):
    task_list = [ecs.list_tasks(cluster = 'devv-test-cluster-01')['taskArns'][task_num]]
    return ecs.describe_tasks(cluster='devv-test-cluster-01', tasks=task_list)['tasks'][0]

def instance_from_arn(arn):
    return(arn.split('/')[-1])

class ECSTask(object):
    _cluster = "devv-test-cluster-01"
    _task_role = "ecsTaskExecutionRole"
    _execution_role = "ecsTaskExecutionRole"
    _task_name = "devv-validator-00"
    _docker_image_name = "devv-bin-full-validator"
    _region = "us-east-2"

    def __init__(self, aws, shard_index, node_index):
        self._aws = aws
        self._task_definition = "arn:aws:ecs:"+self._region+":"+aws.get_iam_account()+":task-definition/"+self._task_name
        self._host_list = []
        self._shard_index = shard_index
        self._node_index = node_index
        self._task_ip_address = "10.0."+str(self._shard_index)+"."+str(self._node_index)
        self._container_instance = self._aws.get_container_instance_by_ip(self._task_ip_address)
        
    def start_task(self):
        task_role_arn = "arn:aws:iam:"+self._aws.get_iam_account()+":role/"+self._task_role
        task_role_arn = "arn:aws:iam:"+self._aws.get_iam_account()+":role/"+self._execution_role
        
        override_dict = {}
        override_dict['name'] = self._docker_image_name
        
        command = ["devcash",
                   "--shard-index", str(self._shard_index),
		   "--node-index", str(self._node_index),
		   "--config", "/etc/devv/validator.conf",
		   "--config", "/etc/devv/default_pass.conf",
		   "--bind-endpoint", "tcp://*:55601"]
        for uri in self._host_list:
            command.append("--host-list", "tcp://"+uri['host']+":"+uri['port'])

        override_dict['command'] = command
        or_param = {'containerOverrides':[override_dict]}
        print(or_param)
        ret = self._aws.get_ecs().start_task(cluster=self._cluster,
                                             taskDefinition=self._task_definition,
                                             overrides=or_param,
                                             containerInstances=[self._container_instance])
        return ret

    def add_host(ip_addr, port):
        self._host_list.append(dict(host=ip_addr, port=port))

        
class AWS(object):
    _session = None
    _cluster = "devv-test-cluster-01"
    _region = "us-east-2"
    _iam_account = "682078287735"

    def __init__(self, session, cluster='devv-test-cluster-01', debug=False):
        self._session = session
        self._debug = debug
        self._ec2 = session.client('ec2')
        self._ecs = session.client('ecs')
        self._cluster = cluster

    def get_ip_by_index(index):
        pass

    def get_iam_account(self):
        return self._iam_account
    
    def get_ecs(self):
        return self._ecs
    
    def get_task_list(self):
        tasks = self._ecs.list_tasks(cluster=self._cluster)['taskArns']
        return tasks
    
    def get_task(self, task_num):
        task_list = [self._ecs.list_tasks(cluster=self._cluster)['taskArns'][task_num]]
        return self._ecs.describe_tasks(cluster=self._cluster, tasks=task_list)['tasks'][0]

    def get_instance(self, task_num):
        task = self.get_task(task_num)
        container_instance_desc = self._ecs.describe_container_instances(cluster=self._cluster,containerInstances=[task['containerInstanceArn']])
        if self._debug:
            print("container_instance")
            print(container_instance_desc)
        container_instance = container_instance_desc['containerInstances'][0]['ec2InstanceId']
        instance = self._ec2.describe_instances(InstanceIds=[container_instance])['Reservations'][0]['Instances'][0]

        if self._debug:
            print("instance")
            print(instance)

        return(instance)

    def get_container_instance_by_ip(self, ip_address):
        container_instances = self._ecs.list_container_instances(cluster=self._cluster)['containerInstanceArns']
        container_inst_desc_list = self._ecs.describe_container_instances(cluster='devv-test-cluster-01', containerInstances=container_instances)['containerInstances']
        ec2_instance_ids = []
        container_instance_dict = {}
        for desc in container_inst_desc_list:
            ec2_instance_ids.append(desc['ec2InstanceId'])
            container_instance_dict[desc['ec2InstanceId']] = desc['containerInstanceArn']

        print("map:")
        print(container_instance_dict)
        
        inst_desc_list = self._ec2.describe_instances(InstanceIds=ec2_instance_ids)['Reservations']

        c_inst = None
        for inst in inst_desc_list:
            trimmed = inst['Instances'][0]
            print("checking "+trimmed['PrivateIpAddress'] +" " + ip_address)
            if trimmed['PrivateIpAddress'] == ip_address:
                if c_inst:
                    raise LookupError("PrivateIpAddress found more than once")
                c_inst = container_instance_dict[trimmed['InstanceId']]
                print("match")
                print(c_inst)

        if c_inst == None:
            raise LookupError("Container for IP Address '"+ip_address+"' not found'")
        
        return c_inst
    
    def get_ip(self, task_num):
        ip = self.get_instance(task_num)['PrivateIpAddress']
        return(ip)

                             
def get_devvnet(filename):
    with open(filename, "r") as f:
        buf = ''.join(f.readlines())
        conf = yaml.load(buf, Loader=yaml.Loader)

    # Set bind_port values
    port = conf['devvnet']['base_port']
    for a in conf['devvnet']['shards']:
        for b in a['process']:
            port = port + 1
            b['bind_port'] = port
    return(conf['devvnet'])


class Devvnet(object):
    _base_port = 0
    _password_file = ""
    _working_dir = ""
    _config_file = ""
    _host = ""
    _host_index_map = {}

    def __init__(self, devvnet):
        self._devvnet = devvnet
        self._shards = []

        self._host_index_map = devvnet['host_index_map']

        try:
            self._base_port = devvnet['base_port']
            self._working_dir = devvnet['working_dir']
            self._password_file = devvnet['password_file']
            self._config_file = devvnet['config_file']
            self._host = devvnet['host']
        except KeyError:
            pass

        current_port = self._base_port
        for i in self._devvnet['shards']:
            print("Adding shard {}".format(i['shard_index']))
            s = Shard(i, self._host_index_map, self._config_file, self._password_file)
            current_port = s.initialize_bind_ports(current_port)
            s.evaluate_hostname(self._host)
            s.connect_shard_nodes()
            self._shards.append(s)


        for i,shard in enumerate(self._shards):
            print("shard: "+ str(shard))
            for i2,node in enumerate(shard.get_nodes()):
                node.grill_raw_subs(shard.get_index())

                for rsub in node.get_raw_subs():
                    print("Getting for shard/name/node_index {}/{}/{}".format(rsub.get_shard_index(), rsub._name, rsub._node_index))
                    n = self.get_shard(rsub.get_shard_index()).get_node(rsub._name, rsub._node_index)
                    node.add_subscriber(n.get_host(), n.get_port())

                node.add_working_dir(self._working_dir)

    def __str__(self):
        s = "Devvnet\n"
        s += "base_port   : "+str(self._base_port)+"\n"
        s += "working_dir : "+str(self._working_dir)+"\n"
        for shard in self._shards:
            s += str(shard)
        return s

    def get_shard(self, index):
        return self._shards[index]

    def get_shards(self):
        return self._shards

    def get_num_nodes(self):
        count = 0
        for shard in self._shards:
            count += shard.get_num_nodes()
        return count


class Shard(object):
    _shard_index = 0;
    _working_dir = ""
    _shard = None
    _nodes = []
    _host = ""

    def __init__(self, shard, host_index_map, config_file, password_file):
        self._shard = shard
        self._nodes = get_nodes(shard, host_index_map)
        self._shard_index = self._shard['shard_index']

        try:
            self._host = self._shard['host']
        except:
            pass

        try:
            self._config_file = self._shard['config_file']
        except:
            self._config_file = config_file

        try:
            self._password_file = self._shard['password_file']
        except:
            self._password_file = password_file

        try:
            self._name = self._shard['t1']
            self._type = "T1"
        except:
            try:
                self._name = self._shard['t2']
                self._type = 'T2'
            except:
                print("Error: Shard type neither Tier1 (t1) or Tier2 (t2)")

        for n in self._nodes:
            n.set_config_file(self._config_file)
            n.set_password_file(self._password_file)
            n.set_type(self._type)

        #self._connect_shard_nodes()

    def __str__(self):
        s = "type: " + self._type + "\n"
        s += "index: " + str(self._shard_index) + "\n"
        for node in self._nodes:
            s += "  " + str(node) + "\n"
        return s

    def initialize_bind_ports(self, port_num):
        current_port = port_num
        for node in self._nodes:
            node.set_port(current_port)
            current_port = current_port + 1
        return current_port

    def connect_shard_nodes(self):
        v_index = [i for i,x in enumerate(self._nodes) if x.is_validator()]
        a_index = [i for i,x in enumerate(self._nodes) if x.is_announcer()]
        r_index = [i for i,x in enumerate(self._nodes) if x.is_repeater()]

        for i in v_index:
            host = self._nodes[i].get_host()
            port = self._nodes[i].get_port()
            #print("setting port to {}".format(port))
            for j in v_index:
                if i == j:
                    continue
                self._nodes[j].add_subscriber(host, port)

            for k in a_index:
                announcer = self._nodes[k]
                if self._nodes[i].get_index() == announcer.get_index():
                    self._nodes[i].add_subscriber(announcer.get_host(), announcer.get_port())
                    break

            for l in r_index:
                #print(type(self._nodes[i].get_index()))
                if self._nodes[i].get_index() == self._nodes[l].get_index():
                    self._nodes[l].add_subscriber(host, port)


    def evaluate_hostname(self, host):
        if self._host == "":
            self._host = host

        for node in self._nodes:
            node.set_host(node.get_host().replace("${node_index}", str(node.get_index())))
            if node.get_host().find("format") > 0:
                #print("formatting")
                node.set_host(eval(node.get_host()))

            node.evaluate_hostname(self._host)

    def get_nodes(self):
        return self._nodes

    def get_num_nodes(self):
        return len(self._nodes)

    def get_node(self, name, index):
        node = [x for x in self._nodes if (x.get_name() == name and x.get_index() == index)]
        if len(node) == 0:
            return None

        if len(node) != 1:
            raise("WOOP: identical nodes? ")

        return node[0]
        #node = [y for y in nodes if y.get_index() == index
        #shard1_validators = [x for x in conf['devvnet']['shards'][1]['process'] if x['name'] == 'validator']

    def get_index(self):
        return self._shard_index


class RawSub():
    def __init__(self, name, shard_index, node_index):
        self._name = name
        self._shard_index = shard_index
        self._node_index = node_index

    def __str__(self):
        sub = "({}:{}:{})".format(self._name, self._shard_index, self._node_index)
        return sub

    def get_shard_index(self):
        return self._shard_index

    def substitute_node_index(self, node_index):
        if self._node_index == "${node_index}":
            self._node_index = int(node_index)
        else:
            print("WARNING: not subbing "+str(self._node_index)+" with "+str(node_index))
        return


class Sub():
    def __init__(self, host, port):
        self._host = host
        self._port = port

    def __str__(self):
        sub = "({}:{})".format(self.get_host(), str(self.get_port()))
        return sub

    def __eq__(self, other):
        if self._host != other.get_host():
            return False
        if self._port != other.get_port():
            return False
        return True

    def get_host(self):
        return self._host

    def set_host(self, hostname):
        self._host = hostname

    def get_port(self):
        return self._port

    def set_port(self, port):
        self._port = port


class Node():
    def __init__(self, shard_index, index, name, host, port = 0):
        self._name = name
        self._type = ""
        self._shard_index = int(shard_index)
        self._index = int(index)
        self._host = host
        self._bind_port = int(port)
        self._subscriber_list = []
        self._raw_sub_list = []
        self._working_dir = ""

    def __str__(self):
        subs = "s["
        for sub in self._subscriber_list:
            subs += str(sub)
        subs += "]"
        rawsubs = "r["
        for rawsub in self._raw_sub_list:
            rawsubs += str(rawsub)
        rawsubs += "]"
        s = "node({}:{}:{}:{}:{}) {} {}".format(self._name, self._index, self._host, self._bind_port, self._working_dir, subs, rawsubs)
        return s

    def add_working_dir(self, directory):
        wd = directory.replace("${name}", self._name)
        wd = wd.replace("${shard_index}", str(self._shard_index))
        wd = wd.replace("${node_index}", str(self.get_index()))
        self._working_dir = wd

    def is_validator(self):
        return(self._name == "validator")

    def is_announcer(self):
        return(self._name == "announcer")

    def is_repeater(self):
        return(self._name == "repeater")

    def add_subscriber(self, host, port):
        self._subscriber_list.append(Sub(host,port))

    def add_raw_sub(self, name, shard_index, node_index):
        rs = RawSub(name,shard_index, node_index)
        #print("adding rawsub: "+str(rs))
        self._raw_sub_list.append(rs)

    def evaluate_hostname(self, host):

        for sub in self._subscriber_list:
            sub.set_host(sub.get_host().replace("${node_index}", str(self.get_index())))
            if sub.get_host().find("format") > 0:
                print("formatting")
                sub.set_host(eval(sub.get_host()))

    def grill_raw_subs(self, shard_index):
        for sub in self._raw_sub_list:
            sub.substitute_node_index(self._index)
            #d = subs.replace("${node_index}", str(self._index))
            print("up "+str(sub))

    def get_raw_subs(self):
        return self._raw_sub_list

    def get_type(self):
        return self._type

    def set_type(self, type):
        self._type = type

    def get_name(self):
        return self._name

    def get_shard_index(self):
        return self._shard_index

    def get_index(self):
        return self._index

    def get_host(self):
        return self._host

    def set_host(self, host):
        self._host = host

    def get_port(self):
        return self._bind_port

    def set_port(self, port):
        self._bind_port = port

    def get_config_file(self):
        return self._config_file

    def set_config_file(self, config):
        self._config_file = config

    def get_password_file(self):
        return self._password_file

    def set_password_file(self, file):
        self._password_file = file

    def get_subscriber_list(self):
        return self._subscriber_list

    def get_working_dir(self):
        return self._working_dir

    def set_working_dir(self, working_dir):
        self._working_dir = working_dir


def get_nodes(yml_dict, host_index_map):
    nodes = []
    shard_index = yml_dict['shard_index']
    try:
        host_index_map = yml_dict['host_index_map']
        print("Using shard's {} for shard {}".format(host_index_map, shard_index))
    except:
        print("Using devvnet host_index_map ({}) for shard {}".format(host_index_map, shard_index))

    for proc in yml_dict['process']:
        try:
            print("creating {} {} processes".format(len(host_index_map), proc['name']))
            for node_index in host_index_map:
                node = Node(shard_index, node_index, proc['name'], host_index_map[node_index], proc['bind_port'])
                try:
                    rawsubs = proc['subscribe']
                    for sub in proc['subscribe']:
                        try:
                            si = sub['shard_index']
                        except:
                            si = yml_dict['shard_index']
                        node.add_raw_sub(sub['name'], si, sub['node_index'])
                except:
                    pass
                nodes.append(node)
        except:
            nodes.append(Node(shard_index, ind, proc['name'], proc['host'], proc['bind_port']))
            print("creating a "+proc['name']+" process")
    return nodes


def run_validator(node):
    # ./devcash --node-index 0 --config ../opt/basic_shard.conf --config ../opt/default_pass.conf --host-list tcp://localhost:56551 --host-list tcp://localhost:56552 --host-list tcp://localhost:57550 --bind-endpoint tcp://*:56550

    cmd = []
    cmd.append("./devcash")
    cmd.extend(["--shard-index", str(node.get_shard_index())])
    cmd.extend(["--node-index", str(node.get_index())])
    cmd.extend(["--num-consensus-threads", "1"])
    cmd.extend(["--num-validator-threads", "1"])
    cmd.extend(["--config", node.get_config_file()])
    cmd.extend(["--config", node.get_password_file()])
    cmd.extend(["--bind-endpoint", "tcp://*:" + str(node.get_port())])
    for sub in node.get_subscriber_list():
        cmd.extend(["--host-list", "tcp://" + sub.get_host() + ":" + str(sub.get_port())])

    return cmd


def run_announcer(node):
    # ./announcer --node-index 0 --shard-index 1 --mode T2 --stop-file /tmp/stop-devcash-announcer.ctl --inn-keys ../opt/inn.key --node-keys ../opt/node.key --bind-endpoint 'tcp://*:50020' --working-dir ../../tmp/working/input/laminar4/ --key-pass password --separate-ops true

    cmd = []
    cmd.append("./pb_announcer")
    cmd.extend(["--shard-index", str(node.get_shard_index())])
    cmd.extend(["--node-index", str(node.get_index())])
    cmd.extend(["--config", node.get_config_file()])
    cmd.extend(["--config", node.get_password_file()])
    cmd.extend(["--mode", node.get_type()])
    cmd.extend(["--bind-endpoint", "tcp://*:" + str(node.get_port())])
    cmd.extend(["--separate-ops", "true"])
    cmd.extend(["--start-delay", str(30)])
    cmd.extend(["--protobuf-endpoint", "tcp://*:" + str(node.get_port() + 100)])

    return cmd


def run_repeater(node):
    # ./repeater --node-index 0 --shard-index 1 --mode T2 --stop-file /tmp/stop-devcash-repeater.ctl --inn-keys ../opt/inn.key --node-keys ../opt/node.key --working-dir ../../tmp/working/output/repeater --host-list tcp://localhost:56550 --key-pass password

    cmd = []
    cmd.append("./repeater")
    cmd.extend(["--shard-index", str(node.get_shard_index())])
    cmd.extend(["--node-index", str(node.get_index())])
    cmd.extend(["--num-consensus-threads", "1"])
    cmd.extend(["--num-validator-threads", "1"])
    cmd.extend(["--config", node.get_config_file()])
    cmd.extend(["--config", node.get_password_file()])
    cmd.extend(["--mode", node.get_type()])
    cmd.extend(["--working-dir", node.get_working_dir()])
    for sub in node.get_subscriber_list():
        cmd.extend(["--host-list", "tcp://" + sub.get_host() + ":" + str(sub.get_port())])

    return cmd


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Launch a devvnet.')
    parser.add_argument('--logdir', action="store", dest='logdir', help='Directory to log output')
    parser.add_argument('--start-processes', action="store_true", dest='start', default=False, help='Start the processes')
    parser.add_argument('--hostname', action="store", dest='hostname', default=None, help='Debugging output')
    parser.add_argument('--debug', action="store_true", dest='start', default=False, help='Debugging output')
    parser.add_argument('devvnet', action="store", help='YAML file describing the devvnet')
    args = parser.parse_args()

    print(args)

    print("logdir: " + args.logdir)
    print("start: " + str(args.start))
    print("hostname: " + str(args.hostname))
    print("devvnet: " + args.devvnet)

    print(validator_json_string)
    
    devvnet = get_devvnet(args.devvnet)
    d = Devvnet(devvnet)
    num_nodes = d.get_num_nodes()

    logfiles = []
    cmds = []
    for s in d.get_shards():
        for n in s.get_nodes():
            if args.hostname and (args.hostname != n.get_host()):
                continue
            if n.get_name() == 'validator':
                cmds.append(run_validator(n))
            elif n.get_name() == 'repeater':
                cmds.append(run_repeater(n))
            elif n.get_name() == 'announcer':
                cmds.append(run_announcer(n))
            logfiles.append(os.path.join(args.logdir,
                                         n.get_name()+"_s"+
                                         str(n.get_shard_index())+"_n"+
                                         str(n.get_index())+"_output.log"))

    ps = []
    for index,cmd in enumerate(cmds):
        print("Node " + str(index) + ":")
        print("   Command: ", *cmd)
        print("   Logfile: ", logfiles[index])
        if args.start:
            with open(logfiles[index], "w") as outfile:
                ps.append(subprocess.Popen(cmd, stdout=outfile, stderr=outfile))
                time.sleep(1.5)

    if args.start:
        for p in ps:
            print("Waiting for nodes ... ctl-c to exit.")
            p.wait()

    print("Goodbye.")
