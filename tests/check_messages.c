#include <riemann/message.h>
#include <riemann/event.h>

START_TEST (test_riemann_message_new)
{
  riemann_message_t *message;

  message = riemann_message_new ();
  ck_assert (message != NULL);

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_free)
{
  errno = 0;
  riemann_message_free (NULL);
  ck_assert_errno (-errno, EINVAL);
}
END_TEST

START_TEST (test_riemann_message_set_events_n)
{
  riemann_message_t *message;
  riemann_event_t *event1, *event2, **events;

  ck_assert_errno (riemann_message_set_events_n (NULL, 0, NULL), EINVAL);

  message = riemann_message_new ();

  ck_assert_errno (riemann_message_set_events_n (message, 0, NULL), ERANGE);
  ck_assert_errno (riemann_message_set_events_n (message, 1, NULL), EINVAL);

  /* --- */

  event1 = riemann_event_new ();
  event2 = riemann_event_new ();

  riemann_event_set (event1,
                     RIEMANN_EVENT_FIELD_HOST, "localhost",
                     RIEMANN_EVENT_FIELD_STATE, "ok",
                     RIEMANN_EVENT_FIELD_NONE);
  riemann_event_set (event2,
                     RIEMANN_EVENT_FIELD_HOST, "localhost",
                     RIEMANN_EVENT_FIELD_SERVICE, "test",
                     RIEMANN_EVENT_FIELD_STATE, "failed",
                     RIEMANN_EVENT_FIELD_NONE);

  events = malloc (sizeof (riemann_event_t *) * 3);
  events[0] = event1;
  events[1] = event2;

  ck_assert (riemann_message_set_events_n (message, 2, events) == 0);
  ck_assert_str_eq (message->events[0]->host, "localhost");
  ck_assert_str_eq (message->events[0]->state, "ok");
  ck_assert_str_eq (message->events[1]->service, "test");
  ck_assert_str_eq (message->events[1]->state, "failed");

  /* --- */

  event1 = riemann_event_new ();
  event2 = riemann_event_new ();

  riemann_event_set (event1,
                     RIEMANN_EVENT_FIELD_HOST, "localhost",
                     RIEMANN_EVENT_FIELD_STATE, "ok",
                     RIEMANN_EVENT_FIELD_NONE);
  riemann_event_set (event2,
                     RIEMANN_EVENT_FIELD_HOST, "localhost",
                     RIEMANN_EVENT_FIELD_SERVICE, "test",
                     RIEMANN_EVENT_FIELD_STATE, "failed",
                     RIEMANN_EVENT_FIELD_NONE);

  events = malloc (sizeof (riemann_event_t *) * 3);
  events[0] = event1;
  events[1] = event2;

  ck_assert (riemann_message_set_events_n (message, 2, events) == 0);
  ck_assert_str_eq (message->events[0]->host, "localhost");
  ck_assert_str_eq (message->events[0]->state, "ok");
  ck_assert_str_eq (message->events[1]->service, "test");
  ck_assert_str_eq (message->events[1]->state, "failed");

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_to_buffer)
{
  riemann_message_t *message;
  riemann_event_t **events;
  uint8_t *buffer;
  size_t len;

  events = malloc (sizeof (riemann_event_t *));

  message = riemann_message_new ();
  events[0] = riemann_event_new ();
  riemann_event_set_one (events[0], SERVICE, "test");
  riemann_message_set_events_n (message, 1, events);

  ck_assert (riemann_message_to_buffer (NULL, NULL) == NULL);
  ck_assert_errno (-errno, EINVAL);

  ck_assert ((buffer = riemann_message_to_buffer (message, NULL)) != NULL);
  free (buffer);

  ck_assert ((buffer = riemann_message_to_buffer (message, &len)) != NULL);
  ck_assert_int_eq (len, 12);
  free (buffer);

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_from_buffer)
{
  riemann_message_t *message, *response;
  uint8_t *buffer;
  size_t len;

  message = riemann_message_create_with_events
    (riemann_event_create (RIEMANN_EVENT_FIELD_SERVICE, "test",
                           RIEMANN_EVENT_FIELD_STATE, "ok",
                           RIEMANN_EVENT_FIELD_NONE),
     NULL);
  buffer = riemann_message_to_buffer (message, &len);

  response = riemann_message_from_buffer (buffer + sizeof (uint32_t),
                                          len - sizeof (uint32_t));
  ck_assert (response != NULL);
  ck_assert_int_eq (response->ok, 0);
  ck_assert_str_eq (response->events[0]->service, "test");
  ck_assert_str_eq (response->events[0]->state, "ok");

  errno = 0;

  ck_assert (riemann_message_from_buffer
             (buffer + sizeof (uint32_t), 0) == NULL);
  ck_assert_errno (-errno, EINVAL);

  ck_assert (riemann_message_from_buffer (NULL, 1) == NULL);
  ck_assert_errno (-errno, EINVAL);

  riemann_message_free (message);
  riemann_message_free (response);
  free (buffer);
}
END_TEST

START_TEST (test_riemann_message_set_events)
{
  riemann_message_t *message;
  int result;

  ck_assert_errno (riemann_message_set_events (NULL), EINVAL);

  message = riemann_message_new ();

  ck_assert_errno (riemann_message_set_events (message, NULL), ERANGE);

  result = riemann_message_set_events
    (message,
     riemann_event_create (RIEMANN_EVENT_FIELD_HOST, "localhost",
                           RIEMANN_EVENT_FIELD_SERVICE, "test",
                           RIEMANN_EVENT_FIELD_STATE, "ok",
                           RIEMANN_EVENT_FIELD_NONE),
     riemann_event_create (RIEMANN_EVENT_FIELD_HOST, "localhost",
                           RIEMANN_EVENT_FIELD_SERVICE, "test-two",
                           RIEMANN_EVENT_FIELD_STATE, "ok",
                           RIEMANN_EVENT_FIELD_NONE),
     NULL);
  ck_assert_int_eq (result, 0);
  ck_assert_str_eq (message->events[0]->service, "test");
  ck_assert_str_eq (message->events[1]->service, "test-two");

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_create_with_events)
{
  riemann_message_t *message;

  ck_assert (riemann_message_create_with_events (NULL) == NULL);
  ck_assert_errno (-errno, EINVAL);

  ck_assert ((message = riemann_message_create_with_events
              (riemann_event_create (RIEMANN_EVENT_FIELD_HOST, "localhost",
                                     RIEMANN_EVENT_FIELD_SERVICE, "test",
                                     RIEMANN_EVENT_FIELD_NONE),
               riemann_event_create (RIEMANN_EVENT_FIELD_SERVICE, "test-two",
                                     RIEMANN_EVENT_FIELD_NONE),
               NULL)) != NULL);
  ck_assert_str_eq (message->events[0]->service, "test");
  ck_assert_str_eq (message->events[1]->service, "test-two");

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_set_query)
{
  riemann_message_t *message;
  riemann_query_t *query = riemann_query_new ("state = \"ok\"");

  message = riemann_message_new ();
  ck_assert_errno (riemann_message_set_query (NULL, NULL), EINVAL);
  ck_assert_errno (riemann_message_set_query (message, NULL), EINVAL);
  ck_assert_errno (riemann_message_set_query (NULL, query), EINVAL);

  ck_assert_errno (riemann_message_set_query (message, query), 0);
  ck_assert_str_eq (message->query->string, "state = \"ok\"");

  query = riemann_query_new ("state = \"fail\"");
  ck_assert_errno (riemann_message_set_query (message, query), 0);
  ck_assert_str_eq (message->query->string, "state = \"fail\"");

  riemann_message_free (message);
}
END_TEST

START_TEST (test_riemann_message_create_with_query)
{
  riemann_message_t *message;

  errno = 0;
  ck_assert (riemann_message_create_with_query (NULL) == NULL);
  ck_assert_errno (-errno, EINVAL);

  message = riemann_message_create_with_query
    (riemann_query_new ("state = \"ok\""));
  ck_assert (message != NULL);
  ck_assert_str_eq (message->query->string, "state = \"ok\"");

  riemann_message_free (message);
}
END_TEST

static TCase *
test_riemann_messages (void)
{
  TCase *test_messages;

  test_messages = tcase_create ("Messages");
  tcase_add_test (test_messages, test_riemann_message_new);
  tcase_add_test (test_messages, test_riemann_message_free);
  tcase_add_test (test_messages, test_riemann_message_set_events_n);
  tcase_add_test (test_messages, test_riemann_message_to_buffer);
  tcase_add_test (test_messages, test_riemann_message_from_buffer);
  tcase_add_test (test_messages, test_riemann_message_set_events);
  tcase_add_test (test_messages, test_riemann_message_create_with_events);
  tcase_add_test (test_messages, test_riemann_message_set_query);
  tcase_add_test (test_messages, test_riemann_message_create_with_query);

  return test_messages;
}
