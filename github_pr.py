#!/usr/bin/env python3
import requests
import json
from serial import Serial
from time import sleep
import sys
from datetime import datetime, timedelta

def fetchPullRequestData(api_token):
  url = 'https://api.github.com/graphql'
  owner = 'ethereum'
  repo = 'solidity'
  #owner = 'chriseth'
  #repo = 'prcontroltest'
  query = { 'query' : """
  {
    repository(owner:"%s", name:"%s") {
      pullRequests(last:40, states:OPEN, orderBy: {field: CREATED_AT, direction: ASC}) {
        edges {
          node {
            number
            title
            url
            mergeable
            createdAt
            commits(last:1) { edges { node { commit { id status { state contexts {
              state context
            } }}}}}
            reviews(last:20) { edges { node { author { login } publishedAt state } } }
          }
        }
      }
    }
  }""" % (owner, repo)}

  headers = {'Authorization': 'token %s' % api_token}

  r = json.loads(requests.post(url=url, json=query, headers=headers).text)
  return r['data']['repository']['pullRequests']['edges']

def getTestSummary(commitStatus):
  ciState = -1
  stateList = ['SUCCESS', 'PENDING', 'FAILURE', 'ERROR']
  states = {'SUCCESS': 0, 'PENDING': 1, 'FAILURE': 2, 'ERROR': 3}
  for context in commitStatus['contexts']:
    if not context['context'].startswith('ci/circleci'):
      continue
    if context['state'] not in states:
      print("UNKNOWN STATE: %s" % context['state'])
      ciState = 4
    else:
      ciState = max(ciState, states[context['state']])
  if ciState < 0:
    ciState = 1
  return (ciState, stateList[ciState])

def getApprovalByUser(reviews):
  approvals = {}
  for review in reviews:
    rc = review['node']
    user = rc['author']['login']
    #print(' -- %s: %s' % (user, rc['state']))
    if rc['state'] == 'COMMENTED':
      continue
    if rc['state'] == 'DISMISSED':
      if user in approvals:
        del approvals[user]
    else:
      approvals[user] = rc['state']
  return approvals

def getApproval(reviews):
  approvalState = -1
  stateList = ['APPROVED', 'PENDING', 'CHANGES_REQUESTED', 'ERROR']
  states = {'APPROVED': 0, 'PENDING': 1, 'CHANGES_REQUESTED': 2, 'ERROR': 3}
  for (user, approval) in getApprovalByUser(reviews).items():
    #print("-> %s: %s" % (user , approval))
    if approval not in states:
      print("UNKNOWN APPROVAL STATE: %s" % approval)
      approvalState = states['ERROR']
    else:
      approvalState = max(approvalState, states[approval])
  if approvalState < 0:
    approvalState = 1
  return (approvalState, stateList[approvalState])

def encodeMergeStatus(status):
  if status == 'MERGEABLE':
    return 0
  elif status == 'CONFLICTING':
    return 2
  else:
    print("UNKNOWN MERGE STATUS: %s" % status)
    return 3

# Returns
#   E: error in script,
#   A: author needs to act,
#   M: can be merged right away,
#   R: reviewer needs to act
#   W: waiting
def combineStatus(merge, ci, approval):
  if merge == 2:
    return 'A'
  if merge == 0:
    if ci == 2 or approval == 2:
      return 'A'
    if ci == 1:
      return 'W'
    if ci == 0:
      if approval == 0:
        return 'M'
      if approval == 1:
        return 'R'
  return 'E'
  

def combinedToColor(status):
  if status == 'W':
    return 128, 128, 0, 0
  elif status == 'A':
    return 255, 0, 0, 1
  elif status == 'R':
    return 128, 128, 0, 1
  elif status == 'M':
    return 0, 255, 0, 1
  elif status == 'P':
    return 0, 0, 255, 0
  return 255, 255, 255, 1

def scaleLinear(t, x, y):
  for i in range(len(x) - 1):
    if t >= x[i] and t < x[i + 1]:
      return (t - x[i]) / (x[i + 1] - x[i]) * (y[i + 1] - y[i]) + y[i]
  if t <= x[0]:
    return y[0]
  else:
    return y[-1]

def fetchPulls(api_token):
  prs = fetchPullRequestData(api_token)
  data = []
  epoch = datetime(1970, 1, 1)
  for pr in prs:
    content = pr['node']
    print("=== %s (%s) ===" % (content['title'], content['number']))
    created = (datetime.strptime(content['createdAt'], "%Y-%m-%dT%H:%M:%SZ") - epoch) / timedelta(seconds=1)
    print("Created: %d (%s)" % (created, content['createdAt']))

    mergeStatus = encodeMergeStatus(content['mergeable'])
    print("Mergeable: %d - %s" % (mergeStatus, content['mergeable']))
    approval = getApproval(content['reviews']['edges'])
    print("Approval: %d - %s" % (approval[0], approval[1]))
    commitStatus = content['commits']['edges'][0]['node']['commit']['status']
    if commitStatus is None:
        commitStatus = {'state': 'UNKNOWN', 'contexts': []}
    print("CI overall: %s" % commitStatus['state'])
    ciState = getTestSummary(commitStatus)
    print("CI (only Circle): %d - %s" % (ciState[0], ciState[1]))
    combined = combineStatus(mergeStatus, ciState[0], approval[0])
    if 'WIP' in content['title']:
      combined = 'P'
    print("-> %d %d %d %d => %s" % (content['number'], mergeStatus, ciState[0], approval[0], combined))
    print('')
    r, g, b, f = combinedToColor(combined)
    data += [(int(content['number']), created, r, g, b, f)]

  times = [t for (i, t, r, g, b, f) in data]
  now = (datetime.utcnow() - epoch) / timedelta(seconds=1)
  # Define picewise linear function such that:
  # last 24 hours =~ upper quarter
  # last week =~ middle half
  # everything older: lower quarter
  x = [min(times), now - 3600 * 24 * 7, now - 3600 * 24, now]
  y = [0, 250, 750, 1000]
  data = [(i, scaleLinear(t, x, y), r, g, b, f) for (i, t, r, g, b, f) in data]

  return data

def send(data, serial):
  encoded = b','.join(b'%d %d %d %d %d %d' % (i, t, r, g, b, f) for (i, t, r, g, b, f) in data)
  print(encoded)
  serial.write(encoded + b'\n')


# For each PR, computes the following 3 status numbers.
# For each of them, we have 0: success, 1: pending, 2: failure, 3: error
#  - mergeable (only 0/2)
#  - ci tests (only circle)
#  - approval
#
# If any of them is in 3, flash white (need to fix this script)
# Apart from that, we have four different combined states:
# - M: can be merged - flash green
# - W: waiting for tests - constant orange
# - R: waiting for review - flash orange
# - A: waiting for author - flash red
#
# In addition to that, if the title contains "WIP", the combined state
# will be
# - P: work in progress - constant blue
#
# If the state of a PR changes, it is brighter for 5 minutes
# (implemented on the microcontroller)

if len(sys.argv) < 3:
  print("Usage: %s <github api token> <serial port>" % sys.argv[0])
  sys.exit(1)

serial = Serial(sys.argv[2])
data = fetchPulls(sys.argv[1])
send(data, serial)
