package com.example.acpigps

import android.content.Context
import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels

class BLEDebugFragment : Fragment() {

    interface BLEDebugListener {
        fun onSendTestClicked()
        fun onDisconnectClicked()
    }

    private var listener: BLEDebugListener? = null

    // ViewModel compartido
    private val viewModel: MainViewModel by activityViewModels()

    // Vistas
    private lateinit var logText: TextView
    private lateinit var bleStatusText: TextView
    private lateinit var btnSendTest: Button
    private lateinit var btnDisconnect: Button

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is BLEDebugListener) {
            listener = context
        } else {
            throw RuntimeException("$context must implement BLEDebugListener")
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.ble_debug_fragment, container, false)
    }


    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        logText = view.findViewById(R.id.ble_log_text)
        bleStatusText = view.findViewById(R.id.ble_status_text)
        btnSendTest = view.findViewById(R.id.btn_send_test)
        btnDisconnect = view.findViewById(R.id.btn_disconnect)

        logText.movementMethod = ScrollingMovementMethod()

        btnSendTest.setOnClickListener { listener?.onSendTestClicked() }
        btnDisconnect.setOnClickListener { listener?.onDisconnectClicked() }

        setupObservers()
    }

    private fun setupObservers() {
        // Observador para los logs
        viewModel.logMessages.observe(viewLifecycleOwner) { logs ->
            logText.text = logs.joinToString("\n")
        }

        // Observador para el estado de la conexión
        viewModel.bleConnectionState.observe(viewLifecycleOwner) { state ->
            bleStatusText.text = when {
                state.isConnected -> "Estado: Conectado"
                state.isScanning -> "Estado: Escaneando..."
                else -> "Estado: Desconectado"
            }
            // Habilitar/deshabilitar botones según el estado
            btnSendTest.isEnabled = state.isConnected
            btnDisconnect.isEnabled = state.isConnected
        }
    }

    override fun onDetach() {
        super.onDetach()
        listener = null
    }
}