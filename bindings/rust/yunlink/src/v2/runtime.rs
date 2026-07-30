struct Raw(*mut sys::yunlink_v2_runtime_t);
unsafe impl Send for Raw {}
unsafe impl Sync for Raw {}

pub struct Runtime {
    raw: Mutex<Raw>,
    sender: broadcast::Sender<Event>,
    callback_context: Box<CallbackContext>,
    callback_token: u64,
}

impl Runtime {
    pub fn start(config: RuntimeConfig) -> Result<Self> {
        if unsafe { sys::yunlink_v2_abi_version() } != 2 {
            return Err(Error { code: 5 });
        }
        let raw = unsafe { sys::yunlink_v2_runtime_create() };
        if raw.is_null() {
            return Err(Error { code: 13 });
        }
        let profiles = config.profiles.iter().map(profile_view).collect::<Vec<_>>();
        let required = config
            .required_profiles
            .iter()
            .map(profile_view)
            .collect::<Vec<_>>();
        let native = sys::yunlink_v2_runtime_config_t {
            struct_size: std::mem::size_of::<sys::yunlink_v2_runtime_config_t>(),
            endpoint_uid: string_view(&config.endpoint_uid),
            display_name: string_view(&config.display_name),
            shared_secret: string_view(&config.shared_secret),
            tcp_listen_port: config.tcp_listen_port,
            profiles: profiles.as_ptr(),
            profile_count: profiles.len(),
            required_profiles: required.as_ptr(),
            required_profile_count: required.len(),
        };
        if let Err(error) = ensure(unsafe { sys::yunlink_v2_runtime_start(raw, &native) }) {
            unsafe { sys::yunlink_v2_runtime_destroy(raw) };
            return Err(error);
        }
        let (sender, _) = broadcast::channel(EVENT_CAPACITY);
        let mut callback_context = Box::new(CallbackContext {
            sender: sender.clone(),
        });
        let callback_token = unsafe {
            sys::yunlink_v2_runtime_subscribe(
                raw,
                Some(event_callback),
                (&mut *callback_context as *mut CallbackContext).cast(),
            )
        };
        if callback_token == 0 {
            unsafe {
                sys::yunlink_v2_runtime_stop(raw);
                sys::yunlink_v2_runtime_destroy(raw);
            }
            return Err(Error { code: 13 });
        }
        Ok(Self {
            raw: Mutex::new(Raw(raw)),
            sender,
            callback_context,
            callback_token,
        })
    }

    fn raw(&self) -> *mut sys::yunlink_v2_runtime_t {
        self.raw.lock().expect("v2 runtime mutex poisoned").0
    }

    pub fn subscribe(&self) -> broadcast::Receiver<Event> {
        self.sender.subscribe()
    }

    pub async fn connect(&self, ip: &str, port: u16) -> Result<Peer> {
        let mut peer = sys::yunlink_v2_peer_t::default();
        ensure(unsafe {
            sys::yunlink_v2_runtime_connect(self.raw(), string_view(ip), port, &mut peer)
        })?;
        Ok(Peer {
            id: c_buffer(&peer.id),
            ip: c_buffer(&peer.ip),
            port: peer.port,
        })
    }

    pub async fn open_session(&self, peer: &Peer) -> Result<u64> {
        let session_id =
            unsafe { sys::yunlink_v2_runtime_open_session(self.raw(), string_view(&peer.id)) };
        (session_id != 0)
            .then_some(session_id)
            .ok_or(Error { code: 8 })
    }

    pub fn close_peer(&self, peer: &Peer) {
        unsafe { sys::yunlink_v2_runtime_close_peer(self.raw(), string_view(&peer.id)) };
    }

    #[allow(clippy::too_many_arguments)]
    pub fn publish(
        &self,
        peer: &Peer,
        session_id: u64,
        family: Family,
        operation: u8,
        target: &Target,
        type_ref: &TypeRef,
        payload: &[u8],
        correlation_id: u64,
        ttl_ms: u32,
        qos: Qos,
        source_entity_uid: &str,
    ) -> Result<MessageHandle> {
        let uid_views = target
            .uids()
            .iter()
            .map(|uid| string_view(uid))
            .collect::<Vec<_>>();
        let native_target = sys::yunlink_v2_target_view_t {
            scope: target.scope(),
            uids: uid_views.as_ptr(),
            uid_count: uid_views.len(),
        };
        let native_type = sys::yunlink_v2_type_ref_view_t {
            profile_id: string_view(&type_ref.profile_id),
            major: type_ref.major,
            minor: type_ref.minor,
            type_name: string_view(&type_ref.type_name),
        };
        let native_payload = sys::yunlink_v2_bytes_view_t {
            data: payload.as_ptr(),
            len: payload.len(),
        };
        let mut handle = sys::yunlink_v2_message_handle_t::default();
        ensure(unsafe {
            sys::yunlink_v2_runtime_publish(
                self.raw(),
                string_view(&peer.id),
                session_id,
                family as u8,
                operation,
                native_target,
                native_type,
                native_payload,
                correlation_id,
                ttl_ms,
                qos as u8,
                string_view(source_entity_uid),
                &mut handle,
            )
        })?;
        Ok(MessageHandle {
            session_id: handle.session_id,
            message_id: handle.message_id,
            correlation_id: handle.correlation_id,
        })
    }

    pub fn session_has_profile(
        &self,
        peer: &Peer,
        session_id: u64,
        profile_id: &str,
        major: u16,
    ) -> bool {
        unsafe {
            sys::yunlink_v2_runtime_session_has_profile(
                self.raw(),
                string_view(&peer.id),
                session_id,
                string_view(profile_id),
                major,
            ) != 0
        }
    }

    pub fn session_endpoint_uid(&self, peer: &Peer, session_id: u64) -> Result<String> {
        let mut uid = [0_i8; 129];
        ensure(unsafe {
            sys::yunlink_v2_runtime_session_endpoint_uid(
                self.raw(),
                string_view(&peer.id),
                session_id,
                uid.as_mut_ptr(),
                uid.len(),
            )
        })?;
        Ok(c_buffer(&uid))
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        let raw = self.raw.lock().expect("v2 runtime mutex poisoned").0;
        unsafe {
            sys::yunlink_v2_runtime_unsubscribe(raw, self.callback_token);
            sys::yunlink_v2_runtime_stop(raw);
            sys::yunlink_v2_runtime_destroy(raw);
        }
        let _ = &self.callback_context;
    }
}
