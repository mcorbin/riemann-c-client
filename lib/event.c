/* riemann/event.c -- Riemann C client library
 * Copyright (C) 2013  Gergely Nagy <algernon@madhouse-project.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <riemann/attribute.h>
#include <riemann/event.h>

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

riemann_event_t *
riemann_event_init (riemann_event_t *event)
{
  if (!event)
    {
      errno = EINVAL;
      return NULL;
    }

  event__init((Event *) event);
  return event;
}

riemann_event_t *
riemann_event_new (void)
{
  riemann_event_t *event;

  event = (riemann_event_t *)malloc (sizeof (riemann_event_t));
  return riemann_event_init (event);
}

void
riemann_event_free (riemann_event_t *event)
{
  if (!event)
    return;

  event__free_unpacked (event, NULL);
}

static void
_riemann_event_set_string (char **str, char *value)
{
  if (*str)
    free (*str);
  *str = strdup (value);
}

static int
_riemann_event_set_va (riemann_event_t *event,
                       riemann_event_field_t first_field, va_list aq)
{
  va_list ap;
  riemann_event_field_t field;

  if (!event)
    return -EINVAL;

  va_copy (ap, aq);
  field = first_field;
  do
    {
      switch (field)
        {
        case RIEMANN_EVENT_FIELD_NONE:
          break;

        case RIEMANN_EVENT_FIELD_TIME:
          event->time = (int64_t) va_arg (ap, int64_t);
          event->has_time = 1;
          break;

        case RIEMANN_EVENT_FIELD_STATE:
          _riemann_event_set_string (&event->state, va_arg (ap, char *));
          break;

        case RIEMANN_EVENT_FIELD_SERVICE:
          _riemann_event_set_string (&event->service, va_arg (ap, char *));
          break;

        case RIEMANN_EVENT_FIELD_HOST:
          _riemann_event_set_string (&event->host, va_arg (ap, char *));
          break;

        case RIEMANN_EVENT_FIELD_DESCRIPTION:
          _riemann_event_set_string (&event->description, va_arg (ap, char *));
          break;

        case RIEMANN_EVENT_FIELD_TAGS:
          va_end (ap);
          return -ENOSYS;

        case RIEMANN_EVENT_FIELD_TTL:
          event->ttl = (float) va_arg (ap, double);
          event->has_ttl = 1;
          break;

        case RIEMANN_EVENT_FIELD_ATTRIBUTES:
          {
            riemann_attribute_t *attrib;
            size_t n;

            for (n = 0; n < event->n_attributes; n++)
              riemann_attribute_free (event->attributes[n]);
            if (event->attributes)
              free (event->attributes);
            event->attributes = NULL;
            event->n_attributes = 0;

            attrib = va_arg (ap, riemann_attribute_t *);
            while (attrib != NULL)
              {
                event->attributes =
                  realloc (event->attributes,
                           sizeof (riemann_attribute_t *) * event->n_attributes);
                event->attributes[event->n_attributes] = attrib;
                event->n_attributes++;
                attrib = va_arg (ap, riemann_attribute_t *);
              }

            break;
          }

        case RIEMANN_EVENT_FIELD_METRIC_S64:
          event->metric_sint64 = va_arg (ap, int64_t);
          event->has_metric_sint64 = 1;
          break;

        case RIEMANN_EVENT_FIELD_METRIC_D:
          event->metric_d = va_arg (ap, double);
          event->has_metric_d = 1;
          break;

        case RIEMANN_EVENT_FIELD_METRIC_F:
          event->metric_f = (float) va_arg (ap, double);
          event->has_metric_f = 1;
          break;

        default:
          va_end (ap);
          return -EPROTO;
        }

      if (field != RIEMANN_EVENT_FIELD_NONE)
        field = va_arg (ap, riemann_event_field_t);
    }
  while (field != RIEMANN_EVENT_FIELD_NONE);

  return 0;
}

int
riemann_event_set (riemann_event_t *event, ...)
{
  va_list ap;
  int r;
  riemann_event_field_t first_field;

  va_start (ap, event);
  first_field = va_arg (ap, riemann_event_field_t);
  r = _riemann_event_set_va (event, first_field, ap);
  va_end (ap);
  return r;
}

riemann_event_t *
riemann_event_create (riemann_event_field_t field, ...)
{
  riemann_event_t *event;
  va_list ap;

  event = riemann_event_new ();
  if (!event)
    return NULL;

  va_start (ap, field);
  if (_riemann_event_set_va (event, field, ap) != 0)
    {
      int e = errno;

      va_end (ap);
      riemann_event_free (event);
      errno = e;
      return NULL;
    }
  va_end (ap);

  return event;
}
