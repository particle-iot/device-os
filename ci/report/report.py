#!/usr/bin/env python
"""Module to upload Python JSON structure to Coveralls.io"""
# -*- coding:utf-8 -*-

from __future__ import absolute_import
from __future__ import print_function

__author__ = 'Lei Xu <eddyxu@gmail.com>'
__version__ = '0.4.2'

__classifiers__ = [
    'Development Status :: 3 - Alpha',
    'Intended Audience :: Developers',
    'License :: OSI Approved :: Apache Software License',
    'Operating System :: OS Independent',
    'Programming Language :: Python',
    'Programming Language :: Python :: 3',
    'Topic :: Internet :: WWW/HTTP',
    'Topic :: Software Development :: Libraries',
    'Topic :: Software Development :: Quality Assurance',
    'Topic :: Utilities',
]

__copyright__ = '2019, %s ' % __author__
__license__ = """
    Copyright %s.
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expressed or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
    """ % __copyright__

# Filename: report.py
# Author: Lei "Eddy" Xu
# Copyright: 2019 Lei Xu
# Description: Module to upload Python JSON structure to Coveralls.io
# License: Apache License, Version 2.0
# Modification History:
# 2019/08/26 - Zachary J. Fields: Added main to support command line file upload

import requests
import json
import os
import sys

URL = os.getenv('COVERALLS_ENDPOINT', 'https://coveralls.io') + "/api/v1/jobs"

class Args:
    skip_ssl_verify = False


def post_report(coverage, args):
    """Post coverage report to coveralls.io."""
    response = requests.post(URL, files={'json_file': json.dumps(coverage)},
                             verify=(not args.skip_ssl_verify))
    try:
        result = response.json()
    except ValueError:
        result = {'error': 'Failure to submit data. '
                  'Response [%(status)s]: %(text)s' % {
                      'status': response.status_code,
                      'text': response.text}}
    print(result)
    if 'error' in result:
        return result['error']
    return 0

def main():
    if (len(sys.argv) < 2):
        print("USAGE: python %s <coverage.json>" % sys.argv[0])
    else:
        args = Args()
        with open(sys.argv[1], 'r') as json_coverage_file:
            # Load coverage details from specified file
            json_coverage_details = json.load(json_coverage_file)

            # Pull environment variables
            if (json_coverage_details['repo_token'] is None):
                json_coverage_details['repo_token'] = os.environ.get('COVERALLS_REPO_TOKEN')
                if (json_coverage_details['repo_token'] is None):
                    print("Environment variable COVERALLS_REPO_TOKEN is required to upload coverage information")
                    exit(-1)

            # Consume Codefresh CI specific environment variables _(if available)_
            if not 'service_job_id' in json_coverage_details:
                json_coverage_details['service_job_id'] = os.environ.get('CF_BUILD_ID')
                if (json_coverage_details['service_job_id'] is not None):
                    json_coverage_details['service_name'] = "Codefresh"
                    json_coverage_details['service_pull_request'] = os.environ.get('CF_PULL_REQUEST_ID')
                else:
                    json_coverage_details['service_name'] = ""
                    del json_coverage_details['service_job_id']

            # Post report to Coveralls.io
            post_report(json_coverage_details, args)

if __name__ == '__main__':
    main()
