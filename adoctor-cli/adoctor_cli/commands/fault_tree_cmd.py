#!/usr/bin/python3
# ******************************************************************************
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
# licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#     http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
# PURPOSE.
# See the Mulan PSL v2 for more details.
# ******************************************************************************/
"""
Description: faultree method's entrance for custom commands
Class:FaultreeCommand
"""
import sys

from adoctor_cli.base_cmd import BaseCommand, str_split, cli_request, add_access_token
from aops_utils.log.log import LOGGER
from aops_utils.restful.helper import make_diag_url
from aops_utils.conf.constant import DIAG_IMPORT_TREE, DIAG_GET_TREE, DIAG_DELETE_TREE
from aops_utils.readconfig import read_json_config_file


class FaultreeCommand(BaseCommand):
    """
    Description: faultree's operations
    Attributes:
        sub_parse: Subcommand parameters
        params: Command line parameters
    """

    def __init__(self):
        """
        Description: Instance initialization
        """
        super().__init__()
        self.add_subcommand(sub_command='faultree',
                            help_desc="faultree's operations")
        self.sub_parse.add_argument(
            '--action',
            help='fault tree actions: add, delete, get',
            nargs='?',
            type=str,
            required=True,
            choices=['add', 'delete', 'get'])

        self.sub_parse.add_argument(
            '--tree_list',
            help='trees',
            nargs='?',
            type=str,
            default="")

        self.sub_parse.add_argument(
            '--conf',
            help='.json config file',
            nargs='?',
            type=str)

        self.sub_parse.add_argument(
            '--export',
            help='export path of the result',
            nargs='?',
            type=str)

        self.sub_parse.add_argument(
            '--description',
            help='The description of the tree.',
            nargs="?",
            type=str,
            default="There is no description."
        )

        add_access_token(self.sub_parse)

    def do_command(self, params):
        """
        Description: Executing command
        Args:
            params: Command line parameters
        """
        action = params.action

        action_dict = {
            'add': self.manage_requests_import_fault_tree,  # /manage/import_faultree
            'delete': self.manage_requests_delete_get_fault_tree,  # /manage/delete_faultree
            'get': self.manage_requests_delete_get_fault_tree  # /manage/get_faultree
        }

        kwargs = {
            "action": action,
            "params": params
        }

        action_dict.get(action)(**kwargs)

    @staticmethod
    def manage_requests_import_fault_tree(**kwargs):
        """
        Description: Executing import command
        Args:
            kwargs(dict): dict of the params
        Returns:
            dict: body of response
        """
        params = kwargs.get('params')
        trees = str_split(params.tree_list) if params.tree_list is not None else []
        if len(trees) != 1:
            LOGGER.error('More than one try is imported by user.')
            print("Only one tree can be accepted or tree name contains ','.")
            print("Please try again.")
            sys.exit(0)
        if params.conf is not None:
            conf = read_json_config_file(params.conf)
        else:
            LOGGER.error('Invalid conf file added by user.')
            print('conf must be included in add command, please try again')
            sys.exit(0)
        if conf is None:
            LOGGER.error('Null conf file added by user.')
            print("The config file is None, please import a valid config file.")
            sys.exit(0)
        diag_url, header = make_diag_url(DIAG_IMPORT_TREE)
        pyload = {
            "trees": [
                {
                    "tree_name": trees[0],
                    "tree_content": conf,
                    "description": params.description
                }
            ]
        }

        return cli_request('POST', diag_url, pyload, header, params.access_token)

    @staticmethod
    def manage_requests_delete_get_fault_tree(**kwargs):
        """
        Description: Executing query or delete command
        Args:
            kwargs(dict): dict of the params
        Returns:
            dict: body of response
        """
        params = kwargs.get('params')
        action = kwargs.get('action')
        trees = str_split(params.tree_list) if params.tree_list is not None else []
        pyload = {"tree_list": trees}

        if action == 'delete':
            diag_url, header = make_diag_url(DIAG_DELETE_TREE)
            return cli_request('DELETE', diag_url, pyload, header, params.access_token)

        diag_url, header = make_diag_url(DIAG_GET_TREE)
        return cli_request('POST', diag_url, pyload, header, params.access_token)