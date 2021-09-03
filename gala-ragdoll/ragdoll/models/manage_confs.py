# coding: utf-8

from __future__ import absolute_import
from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from ragdoll.models.base_model_ import Model
from ragdoll.models.manage_conf import ManageConf
from ragdoll import util


class ManageConfs(Model):
    """NOTE: This class is auto generated by the swagger code generator program.

    Do not edit the class manually.
    """

    def __init__(self, domain_name: str=None, conf_files: List[ManageConf]=None):  # noqa: E501
        """ManageConfs - a model defined in Swagger

        :param domain_name: The domain_name of this ManageConfs.  # noqa: E501
        :type domain_name: str
        :param conf_files: The conf_files of this ManageConfs.  # noqa: E501
        :type conf_files: List[ManageConf]
        """
        self.swagger_types = {
            'domain_name': str,
            'conf_files': List[ManageConf]
        }

        self.attribute_map = {
            'domain_name': 'domainName',
            'conf_files': 'confFiles'
        }

        self._domain_name = domain_name
        self._conf_files = conf_files

    @classmethod
    def from_dict(cls, dikt) -> 'ManageConfs':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The ManageConfs of this ManageConfs.  # noqa: E501
        :rtype: ManageConfs
        """
        return util.deserialize_model(dikt, cls)

    @property
    def domain_name(self) -> str:
        """Gets the domain_name of this ManageConfs.


        :return: The domain_name of this ManageConfs.
        :rtype: str
        """
        return self._domain_name

    @domain_name.setter
    def domain_name(self, domain_name: str):
        """Sets the domain_name of this ManageConfs.


        :param domain_name: The domain_name of this ManageConfs.
        :type domain_name: str
        """

        self._domain_name = domain_name

    @property
    def conf_files(self) -> List[ManageConf]:
        """Gets the conf_files of this ManageConfs.


        :return: The conf_files of this ManageConfs.
        :rtype: List[ManageConf]
        """
        return self._conf_files

    @conf_files.setter
    def conf_files(self, conf_files: List[ManageConf]):
        """Sets the conf_files of this ManageConfs.


        :param conf_files: The conf_files of this ManageConfs.
        :type conf_files: List[ManageConf]
        """

        self._conf_files = conf_files