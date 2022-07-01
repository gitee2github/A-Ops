from abc import ABCMeta
from abc import abstractmethod
from typing import List

from spider.conf.observe_meta import RelationType
from spider.conf.observe_meta import EntityType


class Rule(metaclass=ABCMeta):
    @abstractmethod
    def rule_parsing(self, causal_graph):
        pass


# 规则：如果一个 tcp_link 观测实例 A 和一个 ksliprobe 观测实例 B 属于同一个 process 观测实例 C，则建立 A 到 B 的因果关系。
class SliRule1(Rule):
    def rule_parsing(self, causal_graph):
        topo_edges = causal_graph.topo_edges
        topo_nodes = causal_graph.topo_nodes

        tcp_bt_p = []
        sli_bt_p = []
        for _, edge in topo_edges.items():
            if edge.get('type') != RelationType.BELONGS_TO.value:
                continue
            from_n = topo_nodes.get(edge.get('_from'))
            to_n = topo_nodes.get(edge.get('_to'))
            if to_n.get('type') != EntityType.PROCESS.value:
                continue
            if from_n.get('type') == EntityType.TCP_LINK.value:
                tcp_bt_p.append(edge)
            elif from_n.get('type') == EntityType.REDIS_SLI.value:
                sli_bt_p.append(edge)

        for edge1 in tcp_bt_p:
            for edge2 in sli_bt_p:
                if edge1.get('_to') == edge2.get('_to'):
                    causal_graph.causal_graph.add_edge(edge1.get('_from'), edge2.get('_from'))


# 规则：如果观测实例 A 到观测实例 B 存在 belongs_to 关系，则建立 A 到 B 的因果关系。
class BelongsToRule1(Rule):
    def rule_parsing(self, causal_graph):
        topo_edges = causal_graph.topo_edges
        topo_nodes = causal_graph.topo_nodes
        cause_graph = causal_graph.causal_graph
        for _, edge in topo_edges.items():
            if edge.get('type') != RelationType.BELONGS_TO.value:
                continue
            from_node = topo_nodes.get(edge.get('_from'))
            to_node = topo_nodes.get(edge.get('_to'))
            if from_node.get('type') == EntityType.REDIS_SLI.value and to_node.get('type') == EntityType.PROCESS.value:
                cause_graph.add_edge(edge.get('_to'), edge.get('_from'), **edge)
            else:
                cause_graph.add_edge(edge.get('_from'), edge.get('_to'), **edge)


# 规则：如果观测实例 A 到观测实例 B 存在 runs_on 关系，则建立 B 到 A 的因果关系。
class RunsOnRule1(Rule):
    def rule_parsing(self, causal_graph):
        topo_edges = causal_graph.topo_edges
        cause_graph = causal_graph.causal_graph
        for _, edge in topo_edges.items():
            if edge.get('type') != RelationType.RUNS_ON.value:
                continue
            cause_graph.add_edge(edge.get('_to'), edge.get('_from'), **edge)


class RuleEngine:
    def __init__(self):
        self.rules: List[Rule] = []

    def add_rule(self, rule: Rule):
        self.rules.append(rule)

    def rule_parsing(self, causal_graph):
        for rule in self.rules:
            rule.rule_parsing(causal_graph)


rule_engine = RuleEngine()
rule_engine.add_rule(BelongsToRule1())
rule_engine.add_rule(RunsOnRule1())
rule_engine.add_rule(SliRule1())