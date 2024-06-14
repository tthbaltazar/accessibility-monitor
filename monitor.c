#include <stdio.h>
#include <string.h>

#include <dbus/dbus.h>

int string_starts_with(const char *str, const char *pattern)
{
	while(*str != '\0' && *pattern != '\0')
	{
		if (*str != *pattern)
		{
			return 0;
		}

		pattern += 1;
		str += 1;
	}

	return 1;
}

int is_waiting = 1;

void myPendingCallHandler(DBusPendingCall *pending, void *user_data)
{
	//printf("myPendingCallHandler!\n");

	DBusMessage *reply = dbus_pending_call_steal_reply(pending);

	//printf("interface = %s\n", dbus_message_get_interface(reply));
	//printf("member = %s\n", dbus_message_get_member(reply));
	//printf("signature = %s\n", dbus_message_get_signature(reply));

	dbus_message_unref(reply);

	is_waiting = 0;
}

const char *get_message_type(DBusMessage *msg)
{
	switch(dbus_message_get_type(msg))
	{
		case DBUS_MESSAGE_TYPE_METHOD_CALL: return "METHOD_CALL";
		case DBUS_MESSAGE_TYPE_METHOD_RETURN: return "METHOD_RETURN";
		case DBUS_MESSAGE_TYPE_ERROR: return "ERROR";
		case DBUS_MESSAGE_TYPE_SIGNAL: return "SIGNAL";
	}
	return "INVALID";
}

int main(int argc, char **argv)
{
	//printf("Hello world!\n");

	if (argc < 2)
	{
		printf("must pass buss addres as argument\n");
		return 1;
	}

	const char *bus_addr = argv[1];

	DBusError err;
	dbus_error_init(&err);
	//DBusConnection *connection = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	DBusConnection *connection = dbus_connection_open_private(bus_addr, &err);

	if (connection == NULL)
	{
		printf("Failed to connect\n");
		return 1;
	}

	dbus_bus_register(connection, &err);

	DBusMessage *call = dbus_message_new_method_call(
		"org.freedesktop.DBus",
		"/org/freedesktop/DBus",
		"org.freedesktop.DBus.Monitoring",
		"BecomeMonitor");
	if (call == NULL)
	{
		printf("Failed to create message\n");
		return 1;
	}

	DBusMessageIter iter;
	dbus_message_iter_init_append(call, &iter);

	DBusMessageIter string_array_iter;
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &string_array_iter);
	dbus_message_iter_close_container(&iter, &string_array_iter);

	int flags = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &flags);

	// TODO: switch to blocking call
	DBusPendingCall *pending;
	dbus_connection_send_with_reply(connection, call, &pending, -1);
	dbus_message_unref(call);
	dbus_pending_call_set_notify(pending, myPendingCallHandler, NULL, NULL);
	dbus_pending_call_unref(pending);

	while(is_waiting && dbus_connection_read_write_dispatch(connection, -1))
	{
		// empty
		//printf("loop\n");
	}

	while(1)
	{
		dbus_connection_read_write(connection, -1);
		while(1)
		{
			DBusMessage *msg = dbus_connection_pop_message(connection);
			if (msg == NULL)
			{
				break;
			}
			//printf("pop = %p\n", msg);

			//printf("interface = %s\n", dbus_message_get_interface(msg));
			//printf("member = %s\n", dbus_message_get_member(msg));
			//printf("signature = %s\n", dbus_message_get_signature(msg));

			if (1)
			{
				printf("%s.%s(%s) %s\n",
					dbus_message_get_interface(msg),
					dbus_message_get_member(msg),
					dbus_message_get_signature(msg),
					get_message_type(msg));
			}

			if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL &&
				strcmp(dbus_message_get_interface(msg), "org.freedesktop.DBus.Properties") == 0)
			{
				if (strcmp(dbus_message_get_member(msg), "Get") == 0)
				{
					const char *iface = NULL;
					const char *member = NULL;
					dbus_message_get_args(msg, &err,
						DBUS_TYPE_STRING, &iface,
						DBUS_TYPE_STRING, &member,
						DBUS_TYPE_INVALID);

					if (0)
					{
						printf("%s.%s get_prop\n", iface, member);
					}
				}
				else if (strcmp(dbus_message_get_member(msg), "Set") == 0)
				{
					DBusMessageIter iter;
					dbus_message_iter_init(msg, &iter);

					const char *iface = NULL;
					dbus_message_iter_get_basic(&iter, &iface);
					dbus_message_iter_next(&iter);

					const char *member = NULL;
					dbus_message_iter_get_basic(&iter, &member);
					dbus_message_iter_next(&iter);

					DBusMessageIter sub;
					dbus_message_iter_recurse(&iter, &sub);
					char type = dbus_message_iter_get_arg_type(&sub);
					dbus_message_iter_next(&iter);

					if (0)
					{
						printf("%s %s: set_prop (%c)\n", iface, member, type);
					}
				}
			} else if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL &&
				string_starts_with(dbus_message_get_interface(msg), "org.a11y.atspi.Event"))
			{
				// TODO: check types

				DBusMessageIter iter;
				dbus_message_iter_init(msg, &iter);

				const char *arg0 = NULL;
				dbus_message_iter_get_basic(&iter, &arg0);
				dbus_message_iter_next(&iter);

				int arg1 = 0;
				dbus_message_iter_get_basic(&iter, &arg1);
				dbus_message_iter_next(&iter);

				int arg2 = 0;
				dbus_message_iter_get_basic(&iter, &arg2);
				dbus_message_iter_next(&iter);

				const char *arg3 = "";
				DBusMessageIter sub;
				dbus_message_iter_recurse(&iter, &sub);
				char type = dbus_message_iter_get_arg_type(&sub);
				if (type == DBUS_TYPE_STRING)
				{
					dbus_message_iter_get_basic(&sub, &arg3);
				}
				dbus_message_iter_next(&iter);

				DBusMessageIter props;
				dbus_message_iter_recurse(&iter, &props);
				while (dbus_message_iter_has_next(&props))
				{
					DBusMessageIter entry;
					dbus_message_iter_recurse(&props, &entry);
					{
						const char *key = NULL;
						dbus_message_iter_get_basic(&entry, &key);
						dbus_message_iter_next(&entry);

						printf("prop: %s\n", key);

						DBusMessageIter value;
						dbus_message_iter_recurse(&entry, &value);
						dbus_message_iter_next(&entry);
					}
					dbus_message_iter_next(&props);
				}
				dbus_message_iter_next(&iter);

				//printf("Event: %s %s: \"%s\" %d %d %c(\"%s\")\n",
				//	dbus_message_get_interface(msg),
				//	dbus_message_get_member(msg),
				//	arg0, arg1, arg2, type, arg3);
				//printf("Event: %s %s: \"%s\"\n",
				//	dbus_message_get_interface(msg),
				//	dbus_message_get_member(msg),
				//	arg0);
			}

			dbus_message_unref(msg);
		}
	}

	printf("exiting\n");

	return 0;
}
