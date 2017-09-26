#include "communication_diagnostic.h"

particle::SimpleIntegerDiagnosticData g_rateLimitedEventsCounter(DIAG_ID_CLOUD_RATE_LIMITED_EVENTS, "cloud.rateLimited");
particle::SimpleIntegerDiagnosticData g_unacknowledgedMessageCounter(DIAG_ID_CLOUD_UNACKNOWLEDGED_MESSAGES, "cloud.protoUnack");
