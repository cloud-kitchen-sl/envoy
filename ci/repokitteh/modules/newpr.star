
NEW_CONTRIBUTOR_MESSAGE = """
Hi @%s, welcome and thank you for your contribution.

We will try to review your Pull Request as quickly as possible.

In the meantime, please take a look at the [contribution guidelines](https://github.com/envoyproxy/envoy/blob/main/CONTRIBUTING.md) if you have not done so already.

"""

DRAFT_MESSAGE = """
As a reminder, PRs marked as draft will not be automatically assigned reviewers,
or be handled by maintainer-oncall triage.

Please mark your PR as ready when you want it to be reviewed!
"""


def get_pr_author_association(issue_number):
  return github.call(
    method="GET",
    path="repos/envoyproxy/envoy/pulls/%s" % issue_number)["json"]["author_association"]

def get_pr_is_draft(issue_number):
  return github.call(
    method="GET",
    path="repos/envoyproxy/envoy/pulls/%s" % issue_number)["json"]["draft"]

def is_newcontributor(issue_number):
  return (
    get_pr_author_association(issue_number)
    in ["NONE", "FIRST_TIME_CONTRIBUTOR", "FIRST_TIMER"])

def is_draft(issue_number):
  return (get_pr_is_draft(issue_number) is True)

def should_message_newcontributor(action, issue_number):
  return (
    action == 'opened'
    and is_newcontributor(issue_number))

def warn_about_drafts(action, issue_number):
  return (
    action == 'opened'
    and is_draft(issue_number))

def send_newcontributor_message(sender):
  github.issue_create_comment(NEW_CONTRIBUTOR_MESSAGE % sender)

def send_draft_wip_notice(sender):
  github.issue_create_comment(DRAFT_MESSAGE)

def _pr(action, issue_number, sender, config):
  if should_message_newcontributor(action, issue_number):
    send_newcontributor_message(sender)
  if warn_about_drafts(action, issue_number):
    send_draft_wip_notice()

handlers.pull_request(func=_pr)