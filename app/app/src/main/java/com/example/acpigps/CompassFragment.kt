package com.example.acpigps

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.Animation
import android.view.animation.RotateAnimation
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels

class CompassFragment : Fragment() {

    interface CompassListener {
        fun onSaveLocationClicked()
        fun onToggleWalkClicked()
    }

    private var listener: CompassListener? = null
    private var currentDegree = 0f

    // ViewModel compartido con la Activity
    private val viewModel: MainViewModel by activityViewModels()

    // Vistas
    private lateinit var compassNeedle: ImageView
    private lateinit var directionText: TextView
    private lateinit var btnToggleWalk: Button
    private lateinit var btnSaveLocation: Button
    private lateinit var stepCountText: TextView
    private lateinit var statusText: TextView

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is CompassListener) {
            listener = context
        } else {
            throw RuntimeException("$context must implement CompassListener")
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.compass_fragment, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        compassNeedle = view.findViewById(R.id.compass_needle)
        directionText = view.findViewById(R.id.direction_text)
        btnToggleWalk = view.findViewById(R.id.btn_toggle_walk)
        btnSaveLocation = view.findViewById(R.id.btn_save_location)
        stepCountText = view.findViewById(R.id.step_count_text)
        statusText = view.findViewById(R.id.status_text)

        btnSaveLocation.setOnClickListener { listener?.onSaveLocationClicked() }
        btnToggleWalk.setOnClickListener { listener?.onToggleWalkClicked() }

        setupObservers()
    }

    private fun setupObservers() {
        // Observador para los datos de la brújula (ángulo, dirección, pasos)
        viewModel.compassData.observe(viewLifecycleOwner) { data ->
            animateCompass(data.angle)
            directionText.text = data.direction
            stepCountText.text = "Pasos: ${data.steps}"
        }

        // Observador para el estado de la conexión BLE
        viewModel.bleConnectionState.observe(viewLifecycleOwner) { state ->
            statusText.text = when {
                state.isConnected -> "Estado: Conectado a ${state.deviceName}"
                state.isScanning -> "Estado: Buscando dispositivo..."
                else -> "Estado: Desconectado"
            }
        }

        // Observador para el estado de monitoreo (actualiza el botón)
        viewModel.isMonitoring.observe(viewLifecycleOwner) { isMonitoring ->
            btnToggleWalk.text = if (isMonitoring) "Detener Monitoreo" else "Iniciar Monitoreo"
        }
    }

    private fun animateCompass(angle: Float) {
        if (!::compassNeedle.isInitialized) return
        val rotateAnimation = RotateAnimation(
            currentDegree,
            -angle,
            Animation.RELATIVE_TO_SELF, 0.5f,
            Animation.RELATIVE_TO_SELF, 0.5f
        )
        rotateAnimation.duration = 200
        rotateAnimation.fillAfter = true
        compassNeedle.startAnimation(rotateAnimation)
        currentDegree = -angle
    }

    override fun onDetach() {
        super.onDetach()
        listener = null
    }
}